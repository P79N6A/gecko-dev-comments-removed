



































#include "cairoint.h"

typedef struct _lzw_buf {
    cairo_status_t status;

    unsigned char *data;
    int data_size;
    int num_data;
    uint32_t pending;
    unsigned int pending_bits;
} lzw_buf_t;















static void
_lzw_buf_init (lzw_buf_t *buf, int size)
{
    if (size == 0)
	size = 16;

    buf->status = CAIRO_STATUS_SUCCESS;
    buf->data_size = size;
    buf->num_data = 0;
    buf->pending = 0;
    buf->pending_bits = 0;

    buf->data = malloc (size);
    if (unlikely (buf->data == NULL)) {
	buf->data_size = 0;
	buf->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return;
    }
}





static cairo_status_t
_lzw_buf_grow (lzw_buf_t *buf)
{
    int new_size = buf->data_size * 2;
    unsigned char *new_data;

    if (buf->status)
	return buf->status;

    new_data = NULL;
    
    if (new_size / 2 == buf->data_size)
	new_data = realloc (buf->data, new_size);

    if (unlikely (new_data == NULL)) {
	free (buf->data);
	buf->data_size = 0;
	buf->status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	return buf->status;
    }

    buf->data = new_data;
    buf->data_size = new_size;

    return CAIRO_STATUS_SUCCESS;
}











static void
_lzw_buf_store_bits (lzw_buf_t *buf, uint16_t value, int num_bits)
{
    cairo_status_t status;

    assert (value <= (1 << num_bits) - 1);

    if (buf->status)
	return;

    buf->pending = (buf->pending << num_bits) | value;
    buf->pending_bits += num_bits;

    while (buf->pending_bits >= 8) {
	if (buf->num_data >= buf->data_size) {
	    status = _lzw_buf_grow (buf);
	    if (unlikely (status))
		return;
	}
	buf->data[buf->num_data++] = buf->pending >> (buf->pending_bits - 8);
	buf->pending_bits -= 8;
    }
}








static void
_lzw_buf_store_pending  (lzw_buf_t *buf)
{
    cairo_status_t status;

    if (buf->status)
	return;

    if (buf->pending_bits == 0)
	return;

    assert (buf->pending_bits < 8);

    if (buf->num_data >= buf->data_size) {
	status = _lzw_buf_grow (buf);
	if (unlikely (status))
	    return;
    }

    buf->data[buf->num_data++] = buf->pending << (8 - buf->pending_bits);
    buf->pending_bits = 0;
}


#define LZW_CODE_CLEAR_TABLE	256
#define LZW_CODE_EOD		257
#define LZW_CODE_FIRST		258







typedef uint32_t lzw_symbol_t;

#define LZW_SYMBOL_SET(sym, prev, next)			((sym) = ((prev) << 8)|(next))
#define LZW_SYMBOL_SET_CODE(sym, code, prev, next)	((sym) = ((code << 20)|(prev) << 8)|(next))
#define LZW_SYMBOL_GET_CODE(sym)			(((sym) >> 20))
#define LZW_SYMBOL_GET_PREV(sym)			(((sym) >>  8) & 0x7ff)
#define LZW_SYMBOL_GET_BYTE(sym)			(((sym) >>  0) & 0x0ff)




#define LZW_SYMBOL_KEY_MASK	0x000fffff



#define LZW_SYMBOL_FREE		0x00000000









#define LZW_BITS_MIN		9
#define LZW_BITS_MAX		12
#define LZW_BITS_BOUNDARY(bits)	((1<<(bits))-1)
#define LZW_MAX_SYMBOLS		(1<<LZW_BITS_MAX)

#define LZW_SYMBOL_TABLE_SIZE	9013
#define LZW_SYMBOL_MOD1		LZW_SYMBOL_TABLE_SIZE
#define LZW_SYMBOL_MOD2		9011

typedef struct _lzw_symbol_table {
    lzw_symbol_t table[LZW_SYMBOL_TABLE_SIZE];
} lzw_symbol_table_t;


static void
_lzw_symbol_table_init (lzw_symbol_table_t *table)
{
    memset (table->table, 0, LZW_SYMBOL_TABLE_SIZE * sizeof (lzw_symbol_t));
}












static cairo_bool_t
_lzw_symbol_table_lookup (lzw_symbol_table_t	 *table,
			  lzw_symbol_t		  symbol,
			  lzw_symbol_t		**slot_ret)
{
    
















    int i, idx, step, hash = symbol & LZW_SYMBOL_KEY_MASK;
    lzw_symbol_t candidate;

    idx = hash % LZW_SYMBOL_MOD1;
    step = 0;

    *slot_ret = NULL;
    for (i = 0; i < LZW_SYMBOL_TABLE_SIZE; i++)
    {
	candidate = table->table[idx];
	if (candidate == LZW_SYMBOL_FREE)
	{
	    *slot_ret = &table->table[idx];
	    return FALSE;
	}
	else 
	{
	    if ((candidate & LZW_SYMBOL_KEY_MASK) ==
		(symbol & LZW_SYMBOL_KEY_MASK))
	    {
		*slot_ret = &table->table[idx];
		return TRUE;
	    }
	}

	if (step == 0) {
	    step = hash % LZW_SYMBOL_MOD2;
	    if (step == 0)
		step = 1;
	}

	idx += step;
	if (idx >= LZW_SYMBOL_TABLE_SIZE)
	    idx -= LZW_SYMBOL_TABLE_SIZE;
    }

    return FALSE;
}



















unsigned char *
_cairo_lzw_compress (unsigned char *data, unsigned long *size_in_out)
{
    int bytes_remaining = *size_in_out;
    lzw_buf_t buf;
    lzw_symbol_table_t table;
    lzw_symbol_t symbol, *slot = NULL; 
    int code_next = LZW_CODE_FIRST;
    int code_bits = LZW_BITS_MIN;
    int prev, next = 0; 

    if (*size_in_out == 0)
	return NULL;

    _lzw_buf_init (&buf, *size_in_out);

    _lzw_symbol_table_init (&table);

    
    _lzw_buf_store_bits (&buf, LZW_CODE_CLEAR_TABLE, code_bits);

    while (1) {

	

	prev = *data++;
	bytes_remaining--;
	if (bytes_remaining) {
	    do
	    {
		next = *data++;
		bytes_remaining--;
		LZW_SYMBOL_SET (symbol, prev, next);
		if (_lzw_symbol_table_lookup (&table, symbol, &slot))
		    prev = LZW_SYMBOL_GET_CODE (*slot);
	    } while (bytes_remaining && *slot != LZW_SYMBOL_FREE);
	    if (*slot == LZW_SYMBOL_FREE) {
		data--;
		bytes_remaining++;
	    }
	}

	


	_lzw_buf_store_bits (&buf, prev, code_bits);

	if (bytes_remaining == 0)
	    break;

	LZW_SYMBOL_SET_CODE (*slot, code_next++, prev, next);

	if (code_next > LZW_BITS_BOUNDARY(code_bits))
	{
	    code_bits++;
	    if (code_bits > LZW_BITS_MAX) {
		_lzw_symbol_table_init (&table);
		_lzw_buf_store_bits (&buf, LZW_CODE_CLEAR_TABLE, code_bits - 1);
		code_bits = LZW_BITS_MIN;
		code_next = LZW_CODE_FIRST;
	    }
	}
    }

    
    _lzw_buf_store_bits (&buf, LZW_CODE_EOD, code_bits);

    _lzw_buf_store_pending (&buf);

    
    if (buf.status == CAIRO_STATUS_NO_MEMORY) {
	*size_in_out = 0;
	return NULL;
    }

    assert (buf.status == CAIRO_STATUS_SUCCESS);

    *size_in_out = buf.num_data;
    return buf.data;
}
