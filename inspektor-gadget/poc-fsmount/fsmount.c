#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/mount.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

/* These syscalls have no glibc wrappers */
static int sys_fsopen(const char *fs_name, unsigned int flags)
{
	return syscall(__NR_fsopen, fs_name, flags);
}

static int sys_fsconfig(int fd, unsigned int cmd, const char *key,
			const void *value, int aux)
{
	return syscall(__NR_fsconfig, fd, cmd, key, value, aux);
}

static int sys_fsmount(int fd, unsigned int flags, unsigned int attr_flags)
{
	return syscall(__NR_fsmount, fd, flags, attr_flags);
}

static int sys_move_mount(int from_dfd, const char *from_path,
			  int to_dfd, const char *to_path, unsigned int flags)
{
	return syscall(__NR_move_mount, from_dfd, from_path, to_dfd, to_path,
		       flags);
}

int main(void)
{
	int ret;

	mkdir("/mnt/poc_old_api", 0755);
	mkdir("/mnt/poc_new_api", 0755);

	printf("[PHASE 1] Mounting tmpfs with mount() - VISIBLE\n");
	ret = mount("tmpfs", "/mnt/poc_old_api", "tmpfs", 0, "size=1M");
	if (ret < 0) {
		perror("  mount()");
		printf("  NOTE: Requires CAP_SYS_ADMIN\n");
		return 1;
	}
	printf("  mount() succeeded: tmpfs on /mnt/poc_old_api\n");
	sleep(1);

	printf("  Unmounting with umount()...\n");
	umount("/mnt/poc_old_api");
	sleep(1);

	printf("\n[PHASE 2] Mounting tmpfs with fsopen/fsmount/move_mount - INVISIBLE\n");

	int fs_fd = sys_fsopen("tmpfs", 0);
	if (fs_fd < 0) {
		perror("  fsopen()");
		if (errno == ENOSYS) {
			printf("  NOTE: fsopen requires kernel >= 5.2\n");
		}
		return 1;
	}
	printf("  fsopen(\"tmpfs\") succeeded (fd=%d)\n", fs_fd);

	ret = sys_fsconfig(fs_fd, FSCONFIG_SET_STRING, "size", "1M", 0);
	if (ret < 0) {
		perror("  fsconfig(SET_STRING)");
		close(fs_fd);
		return 1;
	}
	printf("  fsconfig(size=1M) succeeded\n");

	ret = sys_fsconfig(fs_fd, FSCONFIG_CMD_CREATE, NULL, NULL, 0);
	if (ret < 0) {
		perror("  fsconfig(CMD_CREATE)");
		close(fs_fd);
		return 1;
	}
	printf("  fsconfig(CMD_CREATE) succeeded\n");

	int mnt_fd = sys_fsmount(fs_fd, 0, MOUNT_ATTR_NODEV);
	if (mnt_fd < 0) {
		perror("  fsmount()");
		close(fs_fd);
		return 1;
	}
	printf("  fsmount() succeeded (fd=%d)\n", mnt_fd);
	close(fs_fd);

	ret = sys_move_mount(mnt_fd, "", AT_FDCWD, "/mnt/poc_new_api",
			     MOVE_MOUNT_F_EMPTY_PATH);
	if (ret < 0) {
		perror("  move_mount()");
		close(mnt_fd);
		return 1;
	}
	printf("  move_mount() to /mnt/poc_new_api succeeded\n");
	close(mnt_fd);
	sleep(1);

	int fd = open("/mnt/poc_new_api/test_file", O_CREAT | O_WRONLY, 0644);
	if (fd >= 0) {
		write(fd, "mounted via new API\n", 20);
		close(fd);
		printf("  Wrote test file to verify mount is active\n");
	}

	printf("\n[PHASE 3] Unmounting with umount() - VISIBLE\n");
	umount("/mnt/poc_new_api");
	printf("  umount() done\n");

	return 0;
}
