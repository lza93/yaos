#include <kernel/timer.h>
#include <kernel/systick.h>
#include <kernel/task.h>
#include <kernel/lock.h>
#include <error.h>

#ifdef CONFIG_TIMER
struct timer_queue timerq;
struct task *timerd;

static inline int __add_timer(struct ktimer *new)
{
	struct links *curr;
	int stamp = (int)systick;

	new->expires -= 1; /* as it uses time_after() not time_equal */

	if (time_after(new->expires, stamp))
		return -ERR_RANGE;

	new->task = current;

	spin_lock(&timerq.lock);

	/* TODO: make search O(1) or clone() not to run in interrupt context */
	for (curr = timerq.list.next; curr != &timerq.list; curr = curr->next) {
		if (((int)new->expires - stamp) <
				((int)((struct ktimer *)curr)->expires - stamp))
			break;
	}

	if (((int)new->expires - stamp) < ((int)timerq.next - stamp))
		timerq.next = new->expires;

	links_add(&new->list, curr->prev);
	timerq.nr++;

	spin_unlock(&timerq.lock);

	return 0;
}

/* FIXME: Remove the need of controlling interrupts in a timer instance
 * To avoid the race condition, interrupts must be disabled before getting the
 * lock of task to change its state or/and priority. And a timer instance runs
 * in the mode it is invoked by which means that can be called by a task
 * doesn't have the right permission.
 *
 * Typically a timer instance is tied to its own task only. So I take the risk
 * at the moment because can't stay long with being interrupt disabled, but
 * will get back soon. */

static void run_timer()
{
	struct ktimer *timer;
	struct links *curr;
	unsigned int irqflag;
	int tid;

infinite:
	for (curr = timerq.list.next; curr != &timerq.list; curr = curr->next) {
		timer = (struct ktimer *)curr;

		if (time_before(timer->expires, systick)) {
			timerq.next = timer->expires;
			break;
		}

		tid = clone(STACK_SHARED | (get_task_flags(timer->task) &
						TASK_PRIVILEGED), &init);
		if (tid > 0) {
			/* Note that it is running at HIGHEST_PRIORITY just
			 * like its parent, run_timer(). Change the priority to
			 * its own tasks' to do job at the right priority. */
			spin_lock(&current->lock);
			set_task_pri(current, get_task_pri(timer->task));
			spin_unlock(&current->lock);
			schedule();

			if (timer->event)
				timer->event(timer->data);

			/* A trick to enter privileged mode */
			if (!(get_task_flags(current) & TASK_PRIVILEGED)) {
				spin_lock(&current->lock);
				set_task_flags(current, get_task_flags(current)
						| TASK_PRIVILEGED);
				spin_unlock(&current->lock);
				schedule();
			}

			spin_lock_irqsave(&timer->task->lock, irqflag);
			sum_curr_stat(timer->task);
			spin_unlock_irqrestore(&timer->task->lock, irqflag);

			kill((unsigned int)current);
			freeze();
		} else if (tid != 0) /* error if not parent */
			break;

		spin_lock_irqsave(&timerq.lock, irqflag);
		links_del(curr);
		timerq.nr--;
		spin_unlock_irqrestore(&timerq.lock, irqflag);
	}

	yield();
	goto infinite;
}

static void sleep_callback(unsigned int data)
{
	struct task *task = (struct task *)data;

	/* A trick to enter privileged mode */
	if (!(get_task_flags(current) & TASK_PRIVILEGED)) {
		spin_lock(&current->lock);
		set_task_flags(current, get_task_flags(current)
				| TASK_PRIVILEGED);
		spin_unlock(&current->lock);
		schedule();
	}

	if (get_task_state(task) == TASK_SLEEPING) {
		spin_lock(&task->lock);
		set_task_state(task, TASK_RUNNING);
		runqueue_add(task);
		spin_unlock(&task->lock);
	}
}

#include <kernel/init.h>

int __init timer_init()
{
	timerq.nr = 0;
	links_init(&timerq.list);
	lock_init(&timerq.lock);

	if ((timerd = make(TASK_KERNEL | STACK_SHARED, run_timer, &init))
			== NULL)
		return -ERR_ALLOC;

	set_task_pri(timerd, HIGHEST_PRIORITY);

	return 0;
}
#else
unsigned int timer_nr = 0;
#endif /* CONFIG_TIMER */

unsigned int get_timer_nr()
{
#ifdef CONFIG_TIMER
	return timerq.nr;
#else
	return timer_nr;
#endif
}

int add_timer(struct ktimer *new)
{
	return syscall(SYSCALL_TIMER_CREATE, new);
}

#include <foundation.h>

void sleep(unsigned int sec)
{
#ifdef CONFIG_TIMER
	struct ktimer tm;

	tm.expires = systick + sec_to_ticks(sec);
	tm.event = sleep_callback;
	tm.data = (unsigned int)current;
	if (!add_timer(&tm))
		yield();
#else
	unsigned int timeout = systick + sec_to_ticks(sec);
	timer_nr++;
	while (time_before(timeout, systick));
	timer_nr--;
#endif
}

void msleep(unsigned int ms)
{
#ifdef CONFIG_TIMER
	struct ktimer tm;

	tm.expires = systick + msec_to_ticks(ms);
	tm.event = sleep_callback;
	tm.data = (unsigned int)current;
	if (!add_timer(&tm))
		yield();
#else
	unsigned int timeout = systick + msec_to_ticks(ms);
	timer_nr++;
	while (time_before(timeout, systick));
	timer_nr--;
#endif
}

int sys_timer_create(struct ktimer *new)
{
#ifdef CONFIG_TIMER
	return __add_timer(new);
#else
	return -ERR_UNDEF;
#endif
}

void set_timeout(unsigned int *tv, unsigned int ms)
{
	*tv = systick + msec_to_ticks(ms);
}

int is_timeout(unsigned int goal)
{
	if (time_after(goal, systick))
		return 1;

	return 0;
}

#define MHZ		1000000
void udelay(unsigned int us)
{
	volatile unsigned int counter;
	unsigned int goal, stamp;

	stamp = get_sysclk();
	goal  = get_sysclk_freq() / MHZ;
	goal  = goal * us;

	if (goal > get_sysclk_max())
		return;

	do {
		counter = stamp - get_sysclk();
		if ((int)counter < 0)
			counter = get_sysclk_max() - ((int)counter * -1);
	} while (counter < goal);
}

void mdelay(unsigned int ms)
{
	unsigned int tout;
	set_timeout(&tout, ms);
	while (!is_timeout(tout));
}
