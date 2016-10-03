#include <io.h>
#include <kernel/task.h>
#include "fair.h"

/* runqueue's head can be init task. thnik about it */
static DEFINE_LIST_HEAD(cfs_rq);

struct task *cfs_pick_next(struct scheduler *cfs)
{
	struct list *head = cfs->rq;

	if (head->next == head) /* empty */
		return NULL;

	return get_container_of(head->next, struct task, rq);
}

void cfs_rq_add(struct scheduler *cfs, struct task *new)
{
	if (!new || get_task_state(new) || !list_empty(&new->rq))
		return;

	struct list *curr, *head, *to;
	struct task *task;

	head = cfs->rq;
	to   = head->prev;

	for (curr = head->next; curr != head; curr = curr->next) {
		task = get_container_of(curr, struct task, rq);

		if (task->se.vruntime > new->se.vruntime) {
			to = curr->prev;
			break;
		}
	}

	list_add(&new->rq, to);
	cfs->nr_running++;
}

void cfs_rq_del(struct scheduler *cfs, struct task *task)
{
	if (list_empty(&task->rq)) return;

	list_del(&task->rq);
	barrier();
	list_link_init(&task->rq);
	cfs->nr_running--;
}

void cfs_init(struct scheduler *cfs)
{
	cfs->vruntime_base = 0;
	cfs->nr_running    = 0;
	cfs->rq            = (void *)&cfs_rq;
}