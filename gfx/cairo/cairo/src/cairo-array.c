




































#include "cairoint.h"















void
_cairo_array_init (cairo_array_t *array, int element_size)
{
    array->size = 0;
    array->num_elements = 0;
    array->element_size = element_size;
    array->elements = NULL;

    array->is_snapshot = FALSE;
}










void
_cairo_array_init_snapshot (cairo_array_t	*array,
			    const cairo_array_t *other)
{
    array->size = other->size;
    array->num_elements = other->num_elements;
    array->element_size = other->element_size;
    array->elements = other->elements;

    array->is_snapshot = TRUE;
}









void
_cairo_array_fini (cairo_array_t *array)
{
    if (array->is_snapshot)
	return;

    if (array->elements) {
	free (* array->elements);
	free (array->elements);
    }
}









cairo_status_t
_cairo_array_grow_by (cairo_array_t *array, unsigned int additional)
{
    char *new_elements;
    unsigned int old_size = array->size;
    unsigned int required_size = array->num_elements + additional;
    unsigned int new_size;

    assert (! array->is_snapshot);

    
    if (required_size > INT_MAX || required_size < array->num_elements)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (CAIRO_INJECT_FAULT ())
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (required_size <= old_size)
	return CAIRO_STATUS_SUCCESS;

    if (old_size == 0)
	new_size = 1;
    else
	new_size = old_size * 2;

    while (new_size < required_size)
	new_size = new_size * 2;

    if (array->elements == NULL) {
	array->elements = malloc (sizeof (char *));
	if (unlikely (array->elements == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	*array->elements = NULL;
    }

    array->size = new_size;
    new_elements = _cairo_realloc_ab (*array->elements,
			              array->size, array->element_size);

    if (unlikely (new_elements == NULL)) {
	array->size = old_size;
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    *array->elements = new_elements;

    return CAIRO_STATUS_SUCCESS;
}









void
_cairo_array_truncate (cairo_array_t *array, unsigned int num_elements)
{
    assert (! array->is_snapshot);

    if (num_elements < array->num_elements)
	array->num_elements = num_elements;
}























void *
_cairo_array_index (cairo_array_t *array, unsigned int index)
{
    










    if (index == 0 && array->num_elements == 0)
	return NULL;

    assert (index < array->num_elements);

    return (void *) &(*array->elements)[index * array->element_size];
}








void
_cairo_array_copy_element (cairo_array_t *array, int index, void *dst)
{
    memcpy (dst, _cairo_array_index (array, index), array->element_size);
}
















cairo_status_t
_cairo_array_append (cairo_array_t	*array,
		     const void		*element)
{
    assert (! array->is_snapshot);

    return _cairo_array_append_multiple (array, element, 1);
}













cairo_status_t
_cairo_array_append_multiple (cairo_array_t	*array,
			      const void	*elements,
			      int		 num_elements)
{
    cairo_status_t status;
    void *dest;

    assert (! array->is_snapshot);

    status = _cairo_array_allocate (array, num_elements, &dest);
    if (unlikely (status))
	return status;

    memcpy (dest, elements, num_elements * array->element_size);

    return CAIRO_STATUS_SUCCESS;
}














cairo_status_t
_cairo_array_allocate (cairo_array_t	 *array,
		       unsigned int	  num_elements,
		       void		**elements)
{
    cairo_status_t status;

    assert (! array->is_snapshot);

    status = _cairo_array_grow_by (array, num_elements);
    if (unlikely (status))
	return status;

    assert (array->num_elements + num_elements <= array->size);

    *elements = &(*array->elements)[array->num_elements * array->element_size];

    array->num_elements += num_elements;

    return CAIRO_STATUS_SUCCESS;
}








int
_cairo_array_num_elements (cairo_array_t *array)
{
    return array->num_elements;
}









int
_cairo_array_size (cairo_array_t *array)
{
    return array->size;
}



typedef struct {
    const cairo_user_data_key_t *key;
    void *user_data;
    cairo_destroy_func_t destroy;
} cairo_user_data_slot_t;










void
_cairo_user_data_array_init (cairo_user_data_array_t *array)
{
    _cairo_array_init (array, sizeof (cairo_user_data_slot_t));
}








void
_cairo_user_data_array_fini (cairo_user_data_array_t *array)
{
    unsigned int num_slots;

    num_slots = array->num_elements;
    if (num_slots) {
	cairo_user_data_slot_t *slots;

	slots = _cairo_array_index (array, 0);
	do {
	    if (slots->user_data != NULL && slots->destroy != NULL)
		slots->destroy (slots->user_data);
	    slots++;
	} while (--num_slots);
    }

    _cairo_array_fini (array);
}













void *
_cairo_user_data_array_get_data (cairo_user_data_array_t     *array,
				 const cairo_user_data_key_t *key)
{
    int i, num_slots;
    cairo_user_data_slot_t *slots;

    
    if (array == NULL)
	return NULL;

    num_slots = array->num_elements;
    slots = _cairo_array_index (array, 0);
    for (i = 0; i < num_slots; i++) {
	if (slots[i].key == key)
	    return slots[i].user_data;
    }

    return NULL;
}

















cairo_status_t
_cairo_user_data_array_set_data (cairo_user_data_array_t     *array,
				 const cairo_user_data_key_t *key,
				 void			     *user_data,
				 cairo_destroy_func_t	      destroy)
{
    cairo_status_t status;
    int i, num_slots;
    cairo_user_data_slot_t *slots, *slot, new_slot;

    if (user_data) {
	new_slot.key = key;
	new_slot.user_data = user_data;
	new_slot.destroy = destroy;
    } else {
	new_slot.key = NULL;
	new_slot.user_data = NULL;
	new_slot.destroy = NULL;
    }

    slot = NULL;
    num_slots = array->num_elements;
    slots = _cairo_array_index (array, 0);
    for (i = 0; i < num_slots; i++) {
	if (slots[i].key == key) {
	    slot = &slots[i];
	    if (slot->destroy && slot->user_data)
		slot->destroy (slot->user_data);
	    break;
	}
	if (user_data && slots[i].user_data == NULL) {
	    slot = &slots[i];	
	}
    }

    if (slot) {
	*slot = new_slot;
	return CAIRO_STATUS_SUCCESS;
    }

    status = _cairo_array_append (array, &new_slot);
    if (unlikely (status))
	return status;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_user_data_array_copy (cairo_user_data_array_t	*dst,
			     cairo_user_data_array_t	*src)
{
    
    if (dst->num_elements != 0) {
	_cairo_user_data_array_fini (dst);
	_cairo_user_data_array_init (dst);
    }

    if (src->num_elements == 0)
	return CAIRO_STATUS_SUCCESS;

    return _cairo_array_append_multiple (dst,
					 _cairo_array_index (src, 0),
					 src->num_elements);
}

void
_cairo_user_data_array_foreach (cairo_user_data_array_t     *array,
				void (*func) (const void *key,
					      void *elt,
					      void *closure),
				void *closure)
{
    cairo_user_data_slot_t *slots;
    int i, num_slots;

    num_slots = array->num_elements;
    slots = _cairo_array_index (array, 0);
    for (i = 0; i < num_slots; i++) {
	if (slots[i].user_data != NULL)
	    func (slots[i].key, slots[i].user_data, closure);
    }
}
