#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define SYSCALL_RESERVED		0
#define SYSCALL_SCHEDULE		1
#define SYSCALL_TEST			2
#define SYSCALL_OPEN			3
#define SYSCALL_READ			4
#define SYSCALL_WRITE			5
#define SYSCALL_CLOSE			6
#define SYSCALL_NR			7

/* return error code */
#define SYSCALL_UNDEF			1

#ifdef MACHINE
#include <asm/syscall.h>
#endif

#include <types.h>

static inline int open(int id, int mode)
{
	return syscall(SYSCALL_OPEN, id, mode);
}

static inline int read(int id, void *buf, size_t size)
{
	return syscall(SYSCALL_READ, id, buf, size);
}

static inline int write(int id, void *buf, size_t size)
{
	return syscall(SYSCALL_WRITE, id, buf, size);
}

static inline int close(int id)
{
	return syscall(SYSCALL_CLOSE, id);
}

#endif /* __SYSCALL_H__ */
