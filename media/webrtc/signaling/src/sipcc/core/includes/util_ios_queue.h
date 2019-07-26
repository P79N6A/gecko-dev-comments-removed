






































#ifndef _UTIL_IOS_QUEUE_H
#define _UTIL_IOS_QUEUE_H





typedef struct queuetype_ {
    void *qhead;                
    void *qtail;                
    int count;                  
    int maximum;                
} queuetype;

typedef queuetype *queue_ptr_t;

typedef struct nexthelper_
{
    struct nexthelper_ *next;
    unsigned char data[4];
} queue_node, nexthelper, *node_ptr_t;

typedef enum get_node_status_ {
    GET_NODE_FAIL_EMPTY_LIST,
    GET_NODE_FAIL_END_OF_LIST,
    GET_NODE_SUCCESS
} get_node_status;

void queue_init(queuetype *q, int maximum);
void *peekqueuehead(queuetype* q);


int queryqueuedepth(queuetype const *q);
boolean checkqueue(queuetype *q, void *e);
void *p_dequeue(queuetype *qptr);
void p_enqueue(queuetype *qptr, void *eaddr);
void p_requeue(queuetype *qptr, void *eaddr);
void p_swapqueue(queuetype *qptr, void *enew, void *eold);
void p_unqueue(queuetype *q, void *e);
void p_unqueuenext(queuetype *q, void **prev);
void enqueue(queuetype *qptr, void *eaddr);
void *dequeue(queuetype *qptr);
void unqueue(queuetype *q, void *e);
void requeue(queuetype *qptr, void *eaddr);
void *remqueue(queuetype *qptr, void *eaddr, void *paddr);
void insqueue(queuetype *qptr, void *eaddr, void *paddr);
boolean queueBLOCK(queuetype *qptr);
boolean validqueue(queuetype *qptr, boolean print_message);

boolean queue_create_init(queue_ptr_t *queue);
void add_list(queue_ptr_t queue, node_ptr_t node);
get_node_status get_node(queue_ptr_t queue, node_ptr_t *node);
void queue_delete(queue_ptr_t queue);

#endif
