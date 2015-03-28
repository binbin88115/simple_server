#ifndef _SERVER_MQ_H_
#define _SERVER_MQ_H_

struct message {
	int session;
	void * data;
	size_t sz;
};

struct message_queue;

void server_mq_global_push(struct message_queue* queue);
void server_mq_global_pop(struct message_queue* queue);

// 0 for success
int server_mq_push(struct message_queue* queue, struct message* message);
int server_mq_pop(struct message_queue* queue);

#endif // _SERVER_MQ_H_
