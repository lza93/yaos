#include <foundation.h>
#include <stdlib.h>
#include <task.h>

static unsigned *alloc_user_stack(struct task_t *p)
{
	if ( (p->stack_size <= 0) || !(p->stack = (unsigned *)malloc(p->stack_size)) )
		return NULL;

	/* make its stack pointer to point out the highest memory address */
	p->sp = p->stack + ((p->stack_size >> 2) - 1);

	return p->sp;
}

#include <sched.h>

static void load_user_task()
{
	extern int _user_task_list;
	struct task_t *p = (struct task_t *)&_user_task_list;
	int i;

	while (p->flags) {
		/* sanity check */
		if (!p->addr) continue;
		if (!alloc_user_stack(p)) continue;

		/* initialize task register set */
		*(p->sp--) = 0x01000000;		/* psr */
		*(p->sp--) = (unsigned)p->addr;		/* pc */
		for (i = 2; i < (CONTEXT_NR-1); i++) {	/* lr */
			*p->sp = 0;			/* . */
			p->sp--;			/* . */
		}					/* . */
		*p->sp = (unsigned)EXC_RETURN_MSPT;	/* lr */

		/* initial state for all tasks are runnable, add into runqueue */
		runqueue_add(p);
		p++;
	}
}

/* when no task in runqueue, this init_task takes place.
 * do some power saving things */
static void init_task()
{
	load_user_task();

	/* ensure that scheduler is not activated prior
	 * until all things to be ready. */
	schedule_on();

	DBUG(("psr : %x sp : %x int : %x control : %x lr : %x\n", GET_PSR(), GET_SP(), GET_INT(), GET_CON(), GET_LR()));
	DBUG(("PC = %x\n", GET_PC()));
	while (1) {
		printf("init()\n");
		mdelay(500);
	}
}

#include <driver/usart.h>

int main()
{
	usart_open(USART1, (struct usart_t) {
			.brr  = brr2reg(115200, get_sysclk()),
			.gtpr = 0,
			.cr3  = 0,
			.cr2  = 0,
			.cr1  = (1 << 13) /* UE    : USART enable */
			| (1 << 5)        /* RXNEIE: RXNE interrupt enable */
			| (1 << 3)        /* TE    : Transmitter enable */
			| (1 << 2)});     /* RE    : Receiver enable */

	systick_init();
	init_task();

	return 0;
}
