




























typedef int (*heap_higher_priority_func)(void *, void *);
typedef void (*heap_index_func)(void *, int);
typedef void (*heap_for_each_func)(void *, void *);

typedef struct heap_context {
	int array_size;
	int array_size_increment;
	int heap_size;
	void **heap;
	heap_higher_priority_func higher_priority;
	heap_index_func index;
} *heap_context;

#define heap_new	__heap_new
#define heap_free	__heap_free
#define heap_insert	__heap_insert
#define heap_delete	__heap_delete
#define heap_increased	__heap_increased
#define heap_decreased	__heap_decreased
#define heap_element	__heap_element
#define heap_for_each	__heap_for_each

heap_context	heap_new(heap_higher_priority_func, heap_index_func, int);
int		heap_free(heap_context);
int		heap_insert(heap_context, void *);
int		heap_delete(heap_context, int);
int		heap_increased(heap_context, int);
int		heap_decreased(heap_context, int);
void *		heap_element(heap_context, int);
int		heap_for_each(heap_context, heap_for_each_func, void *);
