#include <io.h>
#include <kernel/task.h>

int register_isr(unsigned int nvector, void (*func)())
{
	extern unsigned int _ram_start;
	*((unsigned int *)&_ram_start + nvector) = (unsigned int)func;
	dsb();

	return 0;
}

#include <kernel/lock.h>

void nvic_set(unsigned int nirq, int on)
{
	reg_t *reg;
	unsigned int bit, base;

	bit  = nirq % 32;
	nirq = nirq / 32 * 4;
	base = on? NVIC_BASE : NVIC_BASE + 0x80;
	reg  = (reg_t *)(base + nirq);

	*reg = 1 << bit;

	dsb();
}

void nvic_set_pri(unsigned int nirq, unsigned int pri)
{
	reg_t *reg;
	unsigned int bit;

	bit  = nirq % 4 * 8;
	reg  = (reg_t *)((NVIC_BASE + 0x300) + (nirq / 4 * 4));
	*reg &= ~(0xff << bit);
	*reg |= ((pri & 0xf) << 4) << bit;

	dsb();
}

void __attribute__((naked)) sys_schedule()
{
	SCB_ICSR |= 1 << 28; /* raising pendsv for scheduling */
	__ret();
}

#ifdef CONFIG_SYSCALL
#include <kernel/syscall.h>

#ifdef CONFIG_DEBUG_SYSCALL
unsigned int syscall_count = 0;
#endif

#ifndef CONFIG_SYSCALL_THREAD
#ifdef CONFIG_DEBUG_SYSCALL_NESTED
#include <error.h>
void syscall_nested(int sysnum)
{
	error("syscall %d nested!! current %x %s",
			sysnum, current, current->name);
}
#endif
#endif

void __attribute__((naked)) svc_handler()
{
	__asm__ __volatile__(
#ifdef CONFIG_DEBUG_SYSCALL
			"ldr	r0, =syscall_count	\n\t"
			"ldr	r1, [r0]		\n\t"
			"add	r1, #1			\n\t"
			"str	r1, [r0]		\n\t"
#endif
			"mrs	r12, psp		\n\t"
			/* get the sysnum requested */
			"ldr	r0, [r12]		\n\t"
			"teq	r0, %0			\n\t"
			"beq	sys_schedule		\n\t"
			"push	{lr}			\n\t"
#ifndef CONFIG_SYSCALL_THREAD
#ifdef CONFIG_DEBUG_SYSCALL_NESTED
			"push	{r0-r3, lr}		\n\t"
			"ldr	r1, =current		\n\t"
			"ldr	r2, [r1]		\n\t"
			"ldr	r1, [r2, #4]		\n\t"
			"tst	r1, %2			\n\t"
			"it	ne			\n\t"
			"bne	syscall_nested		\n\t"
			"pop	{r0-r3, lr}		\n\t"
#endif
#endif
			/* save context that are not yet saved by hardware.
			 * you can remove this overhead if not using
			 * `syscall_delegate_atomic()` but using only
			 * CONFING_SYSCALL_THREAD. */
#ifndef CONFIG_SYSCALL_THREAD
			"mov	r1, r12			\n\t"
			"stmdb	r1!, {r4-r11}		\n\t"
			"msr	psp, r1			\n\t"
#endif
			/* if nr >= SYSCALL_NR */
			"cmp	r0, %1			\n\t"
			"it	ge			\n\t"
			/* then nr = 0 */
			"movge	r0, #0			\n\t"
			/* get handler address */
			"ldr	r3, =syscall_table	\n\t"
			"ldr	r3, [r3, r0, lsl #2]	\n\t"
			/* arguments in place */
			"ldr	r0, [r12, #4]		\n\t"
			"ldr	r1, [r12, #8]		\n\t"
			"ldr	r2, [r12, #12]		\n\t"
			"blx	r3			\n\t"
			"mrs	r12, psp		\n\t"
#ifndef CONFIG_SYSCALL_THREAD
			/* check if delegated task */
			"ldr	r1, =current		\n\t"
			"ldr	r2, [r1]		\n\t" /* read flags */
			"ldr	r1, [r2, #4]		\n\t"
			"ands	r1, %2			\n\t"
			/* restore saved context if not */
			"itt	eq			\n\t"
			"ldmiaeq	r12!, {r4-r11}		\n\t"
			"msreq	psp, r12		\n\t"
#endif
			/* store return value */
			"str	r0, [r12]		\n\t"
			"dsb				\n\t"
			"isb				\n\t"
			"pop	{pc}			\n\t"
			:: "I"(SYSCALL_SCHEDULE), "I"(SYSCALL_NR), "I"(TF_SYSCALL)
			: "r12", "memory");
}
#endif /* CONFIG_SYSCALL */
