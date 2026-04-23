#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/io_uring.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

static int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
	return syscall(__NR_io_uring_setup, entries, p);
}

static int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
			  unsigned flags, void *sig)
{
	return syscall(__NR_io_uring_enter, fd, to_submit, min_complete,
		       flags, sig, 0);
}

struct uring_ctx {
	int ring_fd;
	struct io_uring_sqe *sqes;
	struct io_uring_cqe *cqes;
	unsigned *sq_array;
	unsigned *sq_tail;
	unsigned *sq_mask;
	unsigned *cq_head;
	unsigned *cq_tail;
	unsigned *cq_mask;
};

static int setup_uring(struct uring_ctx *ctx)
{
	struct io_uring_params params;
	void *sq_ptr, *cq_ptr;

	memset(&params, 0, sizeof(params));

	ctx->ring_fd = io_uring_setup(4, &params);
	if (ctx->ring_fd < 0) {
		perror("  io_uring_setup");
		return -1;
	}

	size_t sq_ring_sz = params.sq_off.array +
			    params.sq_entries * sizeof(unsigned);
	sq_ptr = mmap(NULL, sq_ring_sz, PROT_READ | PROT_WRITE,
		      MAP_SHARED | MAP_POPULATE, ctx->ring_fd,
		      IORING_OFF_SQ_RING);
	if (sq_ptr == MAP_FAILED) {
		perror("  mmap sq_ring");
		close(ctx->ring_fd);
		return -1;
	}

	ctx->sq_tail = sq_ptr + params.sq_off.tail;
	ctx->sq_mask = sq_ptr + params.sq_off.ring_mask;
	ctx->sq_array = sq_ptr + params.sq_off.array;

	size_t sqes_sz = params.sq_entries * sizeof(struct io_uring_sqe);
	ctx->sqes = mmap(NULL, sqes_sz, PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_POPULATE, ctx->ring_fd,
			 IORING_OFF_SQES);
	if (ctx->sqes == MAP_FAILED) {
		perror("  mmap sqes");
		close(ctx->ring_fd);
		return -1;
	}

	size_t cq_ring_sz = params.cq_off.cqes +
			    params.cq_entries * sizeof(struct io_uring_cqe);
	cq_ptr = mmap(NULL, cq_ring_sz, PROT_READ | PROT_WRITE,
		      MAP_SHARED | MAP_POPULATE, ctx->ring_fd,
		      IORING_OFF_CQ_RING);
	if (cq_ptr == MAP_FAILED) {
		perror("  mmap cq_ring");
		close(ctx->ring_fd);
		return -1;
	}

	ctx->cq_head = cq_ptr + params.cq_off.head;
	ctx->cq_mask = cq_ptr + params.cq_off.ring_mask;
	ctx->cq_tail = cq_ptr + params.cq_off.tail;
	ctx->cqes = cq_ptr + params.cq_off.cqes;

	return 0;
}

static int uring_openat(struct uring_ctx *ctx, const char *path, int flags,
			mode_t mode)
{
	unsigned tail, index;
	struct io_uring_sqe *sqe;

	tail = *ctx->sq_tail;
	index = tail & *ctx->sq_mask;

	sqe = &ctx->sqes[index];
	memset(sqe, 0, sizeof(*sqe));
	sqe->opcode = IORING_OP_OPENAT;
	sqe->fd = AT_FDCWD;
	sqe->addr = (unsigned long)path;
	sqe->len = mode;
	sqe->open_flags = flags;

	ctx->sq_array[index] = index;
	__atomic_store_n(ctx->sq_tail, tail + 1, __ATOMIC_RELEASE);

	int ret = io_uring_enter(ctx->ring_fd, 1, 1, IORING_ENTER_GETEVENTS,
				 NULL);
	if (ret < 0) {
		perror("  io_uring_enter");
		return -1;
	}

	unsigned head = *ctx->cq_head;
	struct io_uring_cqe *cqe = &ctx->cqes[head & *ctx->cq_mask];
	int fd = cqe->res;
	__atomic_store_n(ctx->cq_head, head + 1, __ATOMIC_RELEASE);

	return fd;
}

int main(void)
{
	struct uring_ctx ctx;

	printf("[PHASE 1] Opening file with openat() - VISIBLE\n");
	int fd = openat(AT_FDCWD, "/tmp/VISIBLE_OPENAT",
			O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		printf("  openat() succeeded (fd=%d)\n", fd);
		close(fd);
	} else {
		perror("  openat()");
	}

	sleep(1);

	printf("\n[PHASE 2] Opening file with io_uring IORING_OP_OPENAT - INVISIBLE\n");

	if (setup_uring(&ctx) < 0) {
		fprintf(stderr, "  Failed to set up io_uring.\n");
		fprintf(stderr, "  This may be blocked by seccomp. Try:\n");
		fprintf(stderr, "    --security-opt seccomp=unconfined\n");
		goto phase3;
	}

	fd = uring_openat(&ctx, "/tmp/INVISIBLE_IO_URING",
			  O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		printf("  io_uring OPENAT succeeded (fd=%d)\n", fd);
		write(fd, "opened via io_uring\n", 20);
		close(fd);
	} else {
		fprintf(stderr, "  io_uring OPENAT failed (res=%d)\n", fd);
	}

	close(ctx.ring_fd);

phase3:
	sleep(1);

	printf("\n[PHASE 3] Opening file with openat() again - VISIBLE\n");
	fd = openat(AT_FDCWD, "/tmp/VISIBLE_OPENAT_CONTROL",
		    O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd >= 0) {
		printf("  openat() succeeded (fd=%d)\n", fd);
		close(fd);
	}
	return 0;
}
