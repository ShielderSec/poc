#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/openat2.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

static int sys_openat2(int dirfd, const char *pathname,
		       struct open_how *how, size_t size)
{
	return syscall(__NR_openat2, dirfd, pathname, how, size);
}

int main(void)
{
	int fd = openat(AT_FDCWD, "/tmp/VISIBLE_OPENAT", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		printf("  openat() succeeded (fd=%d)\n", fd);
		close(fd);
	} else {
		perror("  openat()");
	}

	sleep(1);

	struct open_how how;
	memset(&how, 0, sizeof(how));
	how.flags = O_CREAT | O_WRONLY | O_TRUNC;
	how.mode = 0644;
	how.resolve = RESOLVE_NO_SYMLINKS;

	fd = sys_openat2(AT_FDCWD, "/tmp/INVISIBLE_OPENAT2", &how, sizeof(how));
	if (fd >= 0) {
		printf("  openat2() succeeded (fd=%d)\n", fd);
		close(fd);
	} else {
		perror("  openat2()");
		if (fd == -1) {
			printf("  NOTE: openat2 requires kernel >= 5.6\n");
		}
	}

	sleep(1);

	fd = openat(AT_FDCWD, "/tmp/VISIBLE_OPENAT_CONTROL", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		printf("  openat() succeeded (fd=%d)\n", fd);
		close(fd);
	} else {
		perror("  openat()");
	}

	return 0;
}
