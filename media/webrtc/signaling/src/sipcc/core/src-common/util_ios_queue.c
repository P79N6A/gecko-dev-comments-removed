






































#include "cpr_types.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_string.h"
#include "util_ios_queue.h"


void enqueue_inline(queuetype *qptr, void *eaddr);
void *dequeue_inline(queuetype *qptr);
void unqueue_inline(queuetype *q, void *e);
void requeue_inline(queuetype *qptr, void *eaddr);


#define validmem(x) x





void
queue_init (queuetype * q, int maximum)
{
    q->qhead = NULL;
    q->qtail = NULL;
    q->count = 0;
    q->maximum = maximum;
}

#ifdef MGCP_FIXME





void *
peekqueuehead (queuetype *q)
{
    nexthelper *p;

    p = (nexthelper *) q->qhead;    
    return (p);

}
#endif





void
enqueue (queuetype *qptr, void *eaddr)
{
    nexthelper *node;

    node = (nexthelper *) eaddr;
    node->next = NULL;
    enqueue_inline(qptr, (void *)node);
}





void *
dequeue (queuetype *qptr)
{
    return (dequeue_inline(qptr));
}




void
enqueue_inline (queuetype *qptr, void *eaddr)
{
    nexthelper *p, *ptr;

    p = (nexthelper *) qptr->qtail; 
    ptr = (nexthelper *) eaddr;
    



    if ((ptr->next != NULL) || (p == ptr)) {
        err_msg("Queue: Error, queue corrupted %d %d\n", (long)qptr,
                (long) eaddr);
        return;
    }
    if (!p)                     
        qptr->qhead = ptr;
    else                        
        p->next = ptr;          
    qptr->qtail = ptr;          
    qptr->count++;
}




void *
dequeue_inline (queuetype * qptr)
{
    nexthelper *p;

    if (qptr == NULL)
        return (NULL);
    p = (nexthelper *) qptr->qhead; 
    if (p) {                    
        qptr->qhead = p->next;  
        if (!p->next) {
            qptr->qtail = NULL; 
        }
        p->next = NULL;         
    }
    if (p && (--qptr->count < 0) && qptr->maximum) {
        err_msg("Queue: Error, queue count under or over %d\n", qptr->count);
        qptr->count = 0;
    }
    return (p);
}

#ifdef MGCP_FIXME






boolean
queue_create_init (queue_ptr_t *queue)
{
    if (queue) {
        *queue = cpr_malloc(sizeof(queuetype));
        if (*queue == NULL)
            return FALSE;

        queue_init((queuetype *) *queue, 0);
        return TRUE;
    } else {
        return FALSE;
    }
}








void
queue_delete (queue_ptr_t queue)
{
    if (queue) {
        cpr_free((queuetype *) queue);
    }
}









void
add_list (queue_ptr_t queue, node_ptr_t node)
{
    if (queue && node) {
        node->next = NULL;
        enqueue_inline((queuetype *) queue, node);
    }
}














get_node_status
get_node (queue_ptr_t queue, node_ptr_t *node)
{
    if (queue && node) {
        if (!*node) {
            
            *node = peekqueuehead(queue);
            if (!*node)
                return GET_NODE_FAIL_EMPTY_LIST;
            else
                return GET_NODE_SUCCESS;
        } else {
            *node = (*node)->next;
            if (!*node)
                return GET_NODE_FAIL_END_OF_LIST;
            else
                return GET_NODE_SUCCESS;
        }
    } else {
        return GET_NODE_FAIL_EMPTY_LIST;
    }
}
#endif
