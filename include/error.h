#ifndef __ERROR_H__
#define __ERROR_H__

/* return error code */
#define ERR_RETRY		1
#define ERR_UNDEF		2
#define ERR_ALLOC		3
#define ERR_RANGE		4
#define ERR_PARAM		5
#define ERR_PERM		6
#define ERR_DUP			7
#define ERR_PATH		8
#define ERR_ATTR		9
#define ERR_CREATE		10

#define MSG_ERROR		0
#define MSG_SYSTEM		1
#define MSG_DEBUG		2
#define MSG_USER		3

#define panic()			while (1) debug(MSG_ERROR, "panic")
#if ((SOC == bcm2835) || (SOC == bcm2836)) /* syscall to raise scheduler */
#define freeze()		while (1) syscall(SYSCALL_NR)
#else
#define freeze()		while (1) debug(MSG_ERROR, "freezed")
#endif

#include <io.h>
#ifdef CONFIG_DEBUG
#define debug(lv, fmt...)	do {					\
	extern void __putc_debug(int c);				\
	void (*tmp)(int) = putchar;					\
	putchar = __putc_debug;						\
	printk("%04x %s:%s():%d: ", lv, __FILE__, __func__, __LINE__);	\
	printk(fmt);							\
	printk("\n");							\
	putchar = tmp;							\
} while (0)
#else
#define debug(lv, fmt...)	do {					\
	if (lv <= MSG_SYSTEM) {						\
		extern void __putc_debug(int c);			\
		void (*tmp)(int) = putchar;				\
		putchar = __putc_debug;					\
		printk(fmt);						\
		printk("\n");						\
		putchar = tmp;						\
	} else if (lv == MSG_USER) {					\
		printf("%04x %s:%s():%d: ", lv, __FILE__, __func__, __LINE__); \
		putchar('\n');						\
	}								\
} while (0)
#endif

#endif /* __ERROR_H__ */
