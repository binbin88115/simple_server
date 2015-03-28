#include "server_mq.h"

struct message_queue {
	struct message* queue;
};

struct global_queue {
	struct message_queue* head;
	struct message_queue* tail;
	int lock;
};

void server_mq_push(struct message_queue* queue, struct message* message)
{
}

void server_mq_pop(struct message_queue* queue)
{
}