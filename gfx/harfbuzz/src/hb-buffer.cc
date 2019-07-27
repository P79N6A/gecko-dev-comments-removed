




























#include "hb-buffer-private.hh"
#include "hb-utf-private.hh"


#ifndef HB_DEBUG_BUFFER
#define HB_DEBUG_BUFFER (HB_DEBUG+0)
#endif


hb_bool_t
hb_segment_properties_equal (const hb_segment_properties_t *a,
			     const hb_segment_properties_t *b)
{
  return a->direction == b->direction &&
	 a->script    == b->script    &&
	 a->language  == b->language  &&
	 a->reserved1 == b->reserved1 &&
	 a->reserved2 == b->reserved2;

}

unsigned int
hb_segment_properties_hash (const hb_segment_properties_t *p)
{
  return (unsigned int) p->direction ^
	 (unsigned int) p->script ^
	 (intptr_t) (p->language);
}

























bool
hb_buffer_t::enlarge (unsigned int size)
{
  if (unlikely (in_error))
    return false;

  unsigned int new_allocated = allocated;
  hb_glyph_position_t *new_pos = NULL;
  hb_glyph_info_t *new_info = NULL;
  bool separate_out = out_info != info;

  if (unlikely (_hb_unsigned_int_mul_overflows (size, sizeof (info[0]))))
    goto done;

  while (size >= new_allocated)
    new_allocated += (new_allocated >> 1) + 32;

  ASSERT_STATIC (sizeof (info[0]) == sizeof (pos[0]));
  if (unlikely (_hb_unsigned_int_mul_overflows (new_allocated, sizeof (info[0]))))
    goto done;

  new_pos = (hb_glyph_position_t *) realloc (pos, new_allocated * sizeof (pos[0]));
  new_info = (hb_glyph_info_t *) realloc (info, new_allocated * sizeof (info[0]));

done:
  if (unlikely (!new_pos || !new_info))
    in_error = true;

  if (likely (new_pos))
    pos = new_pos;

  if (likely (new_info))
    info = new_info;

  out_info = separate_out ? (hb_glyph_info_t *) pos : info;
  if (likely (!in_error))
    allocated = new_allocated;

  return likely (!in_error);
}

bool
hb_buffer_t::make_room_for (unsigned int num_in,
			    unsigned int num_out)
{
  if (unlikely (!ensure (out_len + num_out))) return false;

  if (out_info == info &&
      out_len + num_out > idx + num_in)
  {
    assert (have_output);

    out_info = (hb_glyph_info_t *) pos;
    memcpy (out_info, info, out_len * sizeof (out_info[0]));
  }

  return true;
}

bool
hb_buffer_t::shift_forward (unsigned int count)
{
  assert (have_output);
  if (unlikely (!ensure (len + count))) return false;

  memmove (info + idx + count, info + idx, (len - idx) * sizeof (info[0]));
  len += count;
  idx += count;

  return true;
}

hb_buffer_t::scratch_buffer_t *
hb_buffer_t::get_scratch_buffer (unsigned int *size)
{
  have_output = false;
  have_positions = false;

  out_len = 0;
  out_info = info;

  assert ((uintptr_t) pos % sizeof (scratch_buffer_t) == 0);
  *size = allocated * sizeof (pos[0]) / sizeof (scratch_buffer_t);
  return (scratch_buffer_t *) (void *) pos;
}





void
hb_buffer_t::reset (void)
{
  if (unlikely (hb_object_is_inert (this)))
    return;

  hb_unicode_funcs_destroy (unicode);
  unicode = hb_unicode_funcs_get_default ();
  replacement = HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT;

  clear ();
}

void
hb_buffer_t::clear (void)
{
  if (unlikely (hb_object_is_inert (this)))
    return;

  hb_segment_properties_t default_props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props = default_props;
  flags = HB_BUFFER_FLAG_DEFAULT;

  content_type = HB_BUFFER_CONTENT_TYPE_INVALID;
  in_error = false;
  have_output = false;
  have_positions = false;

  idx = 0;
  len = 0;
  out_len = 0;
  out_info = info;

  serial = 0;
  memset (allocated_var_bytes, 0, sizeof allocated_var_bytes);
  memset (allocated_var_owner, 0, sizeof allocated_var_owner);

  memset (context, 0, sizeof context);
  memset (context_len, 0, sizeof context_len);
}

void
hb_buffer_t::add (hb_codepoint_t  codepoint,
		  unsigned int    cluster)
{
  hb_glyph_info_t *glyph;

  if (unlikely (!ensure (len + 1))) return;

  glyph = &info[len];

  memset (glyph, 0, sizeof (*glyph));
  glyph->codepoint = codepoint;
  glyph->mask = 1;
  glyph->cluster = cluster;

  len++;
}

void
hb_buffer_t::add_info (const hb_glyph_info_t &glyph_info)
{
  if (unlikely (!ensure (len + 1))) return;

  info[len] = glyph_info;

  len++;
}


void
hb_buffer_t::remove_output (void)
{
  if (unlikely (hb_object_is_inert (this)))
    return;

  have_output = false;
  have_positions = false;

  out_len = 0;
  out_info = info;
}

void
hb_buffer_t::clear_output (void)
{
  if (unlikely (hb_object_is_inert (this)))
    return;

  have_output = true;
  have_positions = false;

  out_len = 0;
  out_info = info;
}

void
hb_buffer_t::clear_positions (void)
{
  if (unlikely (hb_object_is_inert (this)))
    return;

  have_output = false;
  have_positions = true;

  out_len = 0;
  out_info = info;

  memset (pos, 0, sizeof (pos[0]) * len);
}

void
hb_buffer_t::swap_buffers (void)
{
  if (unlikely (in_error)) return;

  assert (have_output);
  have_output = false;

  if (out_info != info)
  {
    hb_glyph_info_t *tmp_string;
    tmp_string = info;
    info = out_info;
    out_info = tmp_string;
    pos = (hb_glyph_position_t *) out_info;
  }

  unsigned int tmp;
  tmp = len;
  len = out_len;
  out_len = tmp;

  idx = 0;
}


void
hb_buffer_t::replace_glyphs (unsigned int num_in,
			     unsigned int num_out,
			     const uint32_t *glyph_data)
{
  if (unlikely (!make_room_for (num_in, num_out))) return;

  merge_clusters (idx, idx + num_in);

  hb_glyph_info_t orig_info = info[idx];
  hb_glyph_info_t *pinfo = &out_info[out_len];
  for (unsigned int i = 0; i < num_out; i++)
  {
    *pinfo = orig_info;
    pinfo->codepoint = glyph_data[i];
    pinfo++;
  }

  idx  += num_in;
  out_len += num_out;
}

void
hb_buffer_t::output_glyph (hb_codepoint_t glyph_index)
{
  if (unlikely (!make_room_for (0, 1))) return;

  out_info[out_len] = info[idx];
  out_info[out_len].codepoint = glyph_index;

  out_len++;
}

void
hb_buffer_t::output_info (const hb_glyph_info_t &glyph_info)
{
  if (unlikely (!make_room_for (0, 1))) return;

  out_info[out_len] = glyph_info;

  out_len++;
}

void
hb_buffer_t::copy_glyph (void)
{
  if (unlikely (!make_room_for (0, 1))) return;

  out_info[out_len] = info[idx];

  out_len++;
}

bool
hb_buffer_t::move_to (unsigned int i)
{
  if (!have_output)
  {
    assert (i <= len);
    idx = i;
    return true;
  }

  assert (i <= out_len + (len - idx));

  if (out_len < i)
  {
    unsigned int count = i - out_len;
    if (unlikely (!make_room_for (count, count))) return false;

    memmove (out_info + out_len, info + idx, count * sizeof (out_info[0]));
    idx += count;
    out_len += count;
  }
  else if (out_len > i)
  {
    
    unsigned int count = out_len - i;

    if (unlikely (idx < count && !shift_forward (count + 32))) return false;

    assert (idx >= count);

    idx -= count;
    out_len -= count;
    memmove (info + idx, out_info + out_len, count * sizeof (out_info[0]));
  }

  return true;
}

void
hb_buffer_t::replace_glyph (hb_codepoint_t glyph_index)
{
  if (unlikely (out_info != info || out_len != idx)) {
    if (unlikely (!make_room_for (1, 1))) return;
    out_info[out_len] = info[idx];
  }
  out_info[out_len].codepoint = glyph_index;

  idx++;
  out_len++;
}


void
hb_buffer_t::set_masks (hb_mask_t    value,
			hb_mask_t    mask,
			unsigned int cluster_start,
			unsigned int cluster_end)
{
  hb_mask_t not_mask = ~mask;
  value &= mask;

  if (!mask)
    return;

  if (cluster_start == 0 && cluster_end == (unsigned int)-1) {
    unsigned int count = len;
    for (unsigned int i = 0; i < count; i++)
      info[i].mask = (info[i].mask & not_mask) | value;
    return;
  }

  unsigned int count = len;
  for (unsigned int i = 0; i < count; i++)
    if (cluster_start <= info[i].cluster && info[i].cluster < cluster_end)
      info[i].mask = (info[i].mask & not_mask) | value;
}

void
hb_buffer_t::reverse_range (unsigned int start,
			    unsigned int end)
{
  unsigned int i, j;

  if (start == end - 1)
    return;

  for (i = start, j = end - 1; i < j; i++, j--) {
    hb_glyph_info_t t;

    t = info[i];
    info[i] = info[j];
    info[j] = t;
  }

  if (pos) {
    for (i = start, j = end - 1; i < j; i++, j--) {
      hb_glyph_position_t t;

      t = pos[i];
      pos[i] = pos[j];
      pos[j] = t;
    }
  }
}

void
hb_buffer_t::reverse (void)
{
  if (unlikely (!len))
    return;

  reverse_range (0, len);
}

void
hb_buffer_t::reverse_clusters (void)
{
  unsigned int i, start, count, last_cluster;

  if (unlikely (!len))
    return;

  reverse ();

  count = len;
  start = 0;
  last_cluster = info[0].cluster;
  for (i = 1; i < count; i++) {
    if (last_cluster != info[i].cluster) {
      reverse_range (start, i);
      start = i;
      last_cluster = info[i].cluster;
    }
  }
  reverse_range (start, i);
}

void
hb_buffer_t::merge_clusters (unsigned int start,
			     unsigned int end)
{
#ifdef HB_NO_MERGE_CLUSTERS
  return;
#endif

  if (unlikely (end - start < 2))
    return;

  unsigned int cluster = info[start].cluster;

  for (unsigned int i = start + 1; i < end; i++)
    cluster = MIN (cluster, info[i].cluster);

  
  while (end < len && info[end - 1].cluster == info[end].cluster)
    end++;

  
  while (idx < start && info[start - 1].cluster == info[start].cluster)
    start--;

  
  if (idx == start)
    for (unsigned i = out_len; i && out_info[i - 1].cluster == info[start].cluster; i--)
      out_info[i - 1].cluster = cluster;

  for (unsigned int i = start; i < end; i++)
    info[i].cluster = cluster;
}
void
hb_buffer_t::merge_out_clusters (unsigned int start,
				 unsigned int end)
{
#ifdef HB_NO_MERGE_CLUSTERS
  return;
#endif

  if (unlikely (end - start < 2))
    return;

  unsigned int cluster = out_info[start].cluster;

  for (unsigned int i = start + 1; i < end; i++)
    cluster = MIN (cluster, out_info[i].cluster);

  
  while (start && out_info[start - 1].cluster == out_info[start].cluster)
    start--;

  
  while (end < out_len && out_info[end - 1].cluster == out_info[end].cluster)
    end++;

  
  if (end == out_len)
    for (unsigned i = idx; i < len && info[i].cluster == out_info[end - 1].cluster; i++)
      info[i].cluster = cluster;

  for (unsigned int i = start; i < end; i++)
    out_info[i].cluster = cluster;
}

void
hb_buffer_t::guess_segment_properties (void)
{
  assert (content_type == HB_BUFFER_CONTENT_TYPE_UNICODE ||
	  (!len && content_type == HB_BUFFER_CONTENT_TYPE_INVALID));

  
  if (props.script == HB_SCRIPT_INVALID) {
    for (unsigned int i = 0; i < len; i++) {
      hb_script_t script = unicode->script (info[i].codepoint);
      if (likely (script != HB_SCRIPT_COMMON &&
		  script != HB_SCRIPT_INHERITED &&
		  script != HB_SCRIPT_UNKNOWN)) {
        props.script = script;
        break;
      }
    }
  }

  
  if (props.direction == HB_DIRECTION_INVALID) {
    props.direction = hb_script_get_horizontal_direction (props.script);
  }

  
  if (props.language == HB_LANGUAGE_INVALID) {
    
    props.language = hb_language_get_default ();
  }
}


static inline void
dump_var_allocation (const hb_buffer_t *buffer)
{
  char buf[80];
  for (unsigned int i = 0; i < 8; i++)
    buf[i] = '0' + buffer->allocated_var_bytes[7 - i];
  buf[8] = '\0';
  DEBUG_MSG (BUFFER, buffer,
	     "Current var allocation: %s",
	     buf);
}

void hb_buffer_t::allocate_var (unsigned int byte_i, unsigned int count, const char *owner)
{
  assert (byte_i < 8 && byte_i + count <= 8);

  if (DEBUG_ENABLED (BUFFER))
    dump_var_allocation (this);
  DEBUG_MSG (BUFFER, this,
	     "Allocating var bytes %d..%d for %s",
	     byte_i, byte_i + count - 1, owner);

  for (unsigned int i = byte_i; i < byte_i + count; i++) {
    assert (!allocated_var_bytes[i]);
    allocated_var_bytes[i]++;
    allocated_var_owner[i] = owner;
  }
}

void hb_buffer_t::deallocate_var (unsigned int byte_i, unsigned int count, const char *owner)
{
  if (DEBUG_ENABLED (BUFFER))
    dump_var_allocation (this);

  DEBUG_MSG (BUFFER, this,
	     "Deallocating var bytes %d..%d for %s",
	     byte_i, byte_i + count - 1, owner);

  assert (byte_i < 8 && byte_i + count <= 8);
  for (unsigned int i = byte_i; i < byte_i + count; i++) {
    assert (allocated_var_bytes[i]);
    assert (0 == strcmp (allocated_var_owner[i], owner));
    allocated_var_bytes[i]--;
  }
}

void hb_buffer_t::assert_var (unsigned int byte_i, unsigned int count, const char *owner)
{
  if (DEBUG_ENABLED (BUFFER))
    dump_var_allocation (this);

  DEBUG_MSG (BUFFER, this,
	     "Asserting var bytes %d..%d for %s",
	     byte_i, byte_i + count - 1, owner);

  assert (byte_i < 8 && byte_i + count <= 8);
  for (unsigned int i = byte_i; i < byte_i + count; i++) {
    assert (allocated_var_bytes[i]);
    assert (0 == strcmp (allocated_var_owner[i], owner));
  }
}

void hb_buffer_t::deallocate_var_all (void)
{
  memset (allocated_var_bytes, 0, sizeof (allocated_var_bytes));
  memset (allocated_var_owner, 0, sizeof (allocated_var_owner));
}












hb_buffer_t *
hb_buffer_create (void)
{
  hb_buffer_t *buffer;

  if (!(buffer = hb_object_create<hb_buffer_t> ()))
    return hb_buffer_get_empty ();

  buffer->reset ();

  return buffer;
}










hb_buffer_t *
hb_buffer_get_empty (void)
{
  static const hb_buffer_t _hb_buffer_nil = {
    HB_OBJECT_HEADER_STATIC,

    const_cast<hb_unicode_funcs_t *> (&_hb_unicode_funcs_nil),
    HB_SEGMENT_PROPERTIES_DEFAULT,
    HB_BUFFER_FLAG_DEFAULT,
    HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT,

    HB_BUFFER_CONTENT_TYPE_INVALID,
    true, 
    true, 
    true  

    
  };

  return const_cast<hb_buffer_t *> (&_hb_buffer_nil);
}











hb_buffer_t *
hb_buffer_reference (hb_buffer_t *buffer)
{
  return hb_object_reference (buffer);
}









void
hb_buffer_destroy (hb_buffer_t *buffer)
{
  if (!hb_object_destroy (buffer)) return;

  hb_unicode_funcs_destroy (buffer->unicode);

  free (buffer->info);
  free (buffer->pos);

  free (buffer);
}















hb_bool_t
hb_buffer_set_user_data (hb_buffer_t        *buffer,
			 hb_user_data_key_t *key,
			 void *              data,
			 hb_destroy_func_t   destroy,
			 hb_bool_t           replace)
{
  return hb_object_set_user_data (buffer, key, data, destroy, replace);
}












void *
hb_buffer_get_user_data (hb_buffer_t        *buffer,
			 hb_user_data_key_t *key)
{
  return hb_object_get_user_data (buffer, key);
}











void
hb_buffer_set_content_type (hb_buffer_t              *buffer,
			    hb_buffer_content_type_t  content_type)
{
  buffer->content_type = content_type;
}











hb_buffer_content_type_t
hb_buffer_get_content_type (hb_buffer_t *buffer)
{
  return buffer->content_type;
}











void
hb_buffer_set_unicode_funcs (hb_buffer_t        *buffer,
			     hb_unicode_funcs_t *unicode_funcs)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  if (!unicode_funcs)
    unicode_funcs = hb_unicode_funcs_get_default ();


  hb_unicode_funcs_reference (unicode_funcs);
  hb_unicode_funcs_destroy (buffer->unicode);
  buffer->unicode = unicode_funcs;
}











hb_unicode_funcs_t *
hb_buffer_get_unicode_funcs (hb_buffer_t        *buffer)
{
  return buffer->unicode;
}










void
hb_buffer_set_direction (hb_buffer_t    *buffer,
			 hb_direction_t  direction)

{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->props.direction = direction;
}











hb_direction_t
hb_buffer_get_direction (hb_buffer_t    *buffer)
{
  return buffer->props.direction;
}










void
hb_buffer_set_script (hb_buffer_t *buffer,
		      hb_script_t  script)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->props.script = script;
}











hb_script_t
hb_buffer_get_script (hb_buffer_t *buffer)
{
  return buffer->props.script;
}










void
hb_buffer_set_language (hb_buffer_t   *buffer,
			hb_language_t  language)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->props.language = language;
}











hb_language_t
hb_buffer_get_language (hb_buffer_t *buffer)
{
  return buffer->props.language;
}










void
hb_buffer_set_segment_properties (hb_buffer_t *buffer,
				  const hb_segment_properties_t *props)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->props = *props;
}










void
hb_buffer_get_segment_properties (hb_buffer_t *buffer,
				  hb_segment_properties_t *props)
{
  *props = buffer->props;
}











void
hb_buffer_set_flags (hb_buffer_t       *buffer,
		     hb_buffer_flags_t  flags)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->flags = flags;
}











hb_buffer_flags_t
hb_buffer_get_flags (hb_buffer_t *buffer)
{
  return buffer->flags;
}











void
hb_buffer_set_replacement_codepoint (hb_buffer_t    *buffer,
				     hb_codepoint_t  replacement)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return;

  buffer->replacement = replacement;
}











hb_codepoint_t
hb_buffer_get_replacement_codepoint (hb_buffer_t    *buffer)
{
  return buffer->replacement;
}










void
hb_buffer_reset (hb_buffer_t *buffer)
{
  buffer->reset ();
}









void
hb_buffer_clear_contents (hb_buffer_t *buffer)
{
  buffer->clear ();
}












hb_bool_t
hb_buffer_pre_allocate (hb_buffer_t *buffer, unsigned int size)
{
  return buffer->ensure (size);
}











hb_bool_t
hb_buffer_allocation_successful (hb_buffer_t  *buffer)
{
  return !buffer->in_error;
}











void
hb_buffer_add (hb_buffer_t    *buffer,
	       hb_codepoint_t  codepoint,
	       unsigned int    cluster)
{
  buffer->add (codepoint, cluster);
  buffer->clear_context (1);
}












hb_bool_t
hb_buffer_set_length (hb_buffer_t  *buffer,
		      unsigned int  length)
{
  if (unlikely (hb_object_is_inert (buffer)))
    return length == 0;

  if (!buffer->ensure (length))
    return false;

  
  if (length > buffer->len) {
    memset (buffer->info + buffer->len, 0, sizeof (buffer->info[0]) * (length - buffer->len));
    if (buffer->have_positions)
      memset (buffer->pos + buffer->len, 0, sizeof (buffer->pos[0]) * (length - buffer->len));
  }

  buffer->len = length;

  if (!length)
  {
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_INVALID;
    buffer->clear_context (0);
  }
  buffer->clear_context (1);

  return true;
}











unsigned int
hb_buffer_get_length (hb_buffer_t *buffer)
{
  return buffer->len;
}













hb_glyph_info_t *
hb_buffer_get_glyph_infos (hb_buffer_t  *buffer,
                           unsigned int *length)
{
  if (length)
    *length = buffer->len;

  return (hb_glyph_info_t *) buffer->info;
}













hb_glyph_position_t *
hb_buffer_get_glyph_positions (hb_buffer_t  *buffer,
                               unsigned int *length)
{
  if (!buffer->have_positions)
    buffer->clear_positions ();

  if (length)
    *length = buffer->len;

  return (hb_glyph_position_t *) buffer->pos;
}









void
hb_buffer_reverse (hb_buffer_t *buffer)
{
  buffer->reverse ();
}











void
hb_buffer_reverse_clusters (hb_buffer_t *buffer)
{
  buffer->reverse_clusters ();
}

























void
hb_buffer_guess_segment_properties (hb_buffer_t *buffer)
{
  buffer->guess_segment_properties ();
}

template <bool validate, typename T>
static inline void
hb_buffer_add_utf (hb_buffer_t  *buffer,
		   const T      *text,
		   int           text_length,
		   unsigned int  item_offset,
		   int           item_length)
{
  typedef hb_utf_t<T, true> utf_t;
  const hb_codepoint_t replacement = buffer->replacement;

  assert (buffer->content_type == HB_BUFFER_CONTENT_TYPE_UNICODE ||
	  (!buffer->len && buffer->content_type == HB_BUFFER_CONTENT_TYPE_INVALID));

  if (unlikely (hb_object_is_inert (buffer)))
    return;

  if (text_length == -1)
    text_length = utf_t::strlen (text);

  if (item_length == -1)
    item_length = text_length - item_offset;

  buffer->ensure (buffer->len + item_length * sizeof (T) / 4);

  






  if (!buffer->len && item_offset > 0)
  {
    
    buffer->clear_context (0);
    const T *prev = text + item_offset;
    const T *start = text;
    while (start < prev && buffer->context_len[0] < buffer->CONTEXT_LENGTH)
    {
      hb_codepoint_t u;
      prev = utf_t::prev (prev, start, &u, replacement);
      buffer->context[0][buffer->context_len[0]++] = u;
    }
  }

  const T *next = text + item_offset;
  const T *end = next + item_length;
  while (next < end)
  {
    hb_codepoint_t u;
    const T *old_next = next;
    next = utf_t::next (next, end, &u, replacement);
    buffer->add (u, old_next - (const T *) text);
  }

  
  buffer->clear_context (1);
  end = text + text_length;
  while (next < end && buffer->context_len[1] < buffer->CONTEXT_LENGTH)
  {
    hb_codepoint_t u;
    next = utf_t::next (next, end, &u, replacement);
    buffer->context[1][buffer->context_len[1]++] = u;
  }

  buffer->content_type = HB_BUFFER_CONTENT_TYPE_UNICODE;
}













void
hb_buffer_add_utf8 (hb_buffer_t  *buffer,
		    const char   *text,
		    int           text_length,
		    unsigned int  item_offset,
		    int           item_length)
{
  hb_buffer_add_utf<true> (buffer, (const uint8_t *) text, text_length, item_offset, item_length);
}













void
hb_buffer_add_utf16 (hb_buffer_t    *buffer,
		     const uint16_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length)
{
  hb_buffer_add_utf<true> (buffer, text, text_length, item_offset, item_length);
}













void
hb_buffer_add_utf32 (hb_buffer_t    *buffer,
		     const uint32_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length)
{
  hb_buffer_add_utf<true> (buffer, text, text_length, item_offset, item_length);
}













void
hb_buffer_add_codepoints (hb_buffer_t          *buffer,
			  const hb_codepoint_t *text,
			  int                   text_length,
			  unsigned int          item_offset,
			  int                   item_length)
{
  hb_buffer_add_utf<false> (buffer, text, text_length, item_offset, item_length);
}


static int
compare_info_codepoint (const hb_glyph_info_t *pa,
			const hb_glyph_info_t *pb)
{
  return (int) pb->codepoint - (int) pa->codepoint;
}

static inline void
normalize_glyphs_cluster (hb_buffer_t *buffer,
			  unsigned int start,
			  unsigned int end,
			  bool backward)
{
  hb_glyph_position_t *pos = buffer->pos;

  
  hb_position_t total_x_advance = 0, total_y_advance = 0;
  for (unsigned int i = start; i < end; i++)
  {
    total_x_advance += pos[i].x_advance;
    total_y_advance += pos[i].y_advance;
  }

  hb_position_t x_advance = 0, y_advance = 0;
  for (unsigned int i = start; i < end; i++)
  {
    pos[i].x_offset += x_advance;
    pos[i].y_offset += y_advance;

    x_advance += pos[i].x_advance;
    y_advance += pos[i].y_advance;

    pos[i].x_advance = 0;
    pos[i].y_advance = 0;
  }

  if (backward)
  {
    
    pos[end - 1].x_advance = total_x_advance;
    pos[end - 1].y_advance = total_y_advance;

    hb_bubble_sort (buffer->info + start, end - start - 1, compare_info_codepoint, buffer->pos + start);
  } else {
    
    pos[start].x_advance += total_x_advance;
    pos[start].y_advance += total_y_advance;
    for (unsigned int i = start + 1; i < end; i++) {
      pos[i].x_offset -= total_x_advance;
      pos[i].y_offset -= total_y_advance;
    }
    hb_bubble_sort (buffer->info + start + 1, end - start - 1, compare_info_codepoint, buffer->pos + start + 1);
  }
}









void
hb_buffer_normalize_glyphs (hb_buffer_t *buffer)
{
  assert (buffer->have_positions);
  assert (buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS);

  bool backward = HB_DIRECTION_IS_BACKWARD (buffer->props.direction);

  unsigned int count = buffer->len;
  if (unlikely (!count)) return;
  hb_glyph_info_t *info = buffer->info;

  unsigned int start = 0;
  unsigned int end;
  for (end = start + 1; end < count; end++)
    if (info[start].cluster != info[end].cluster) {
      normalize_glyphs_cluster (buffer, start, end, backward);
      start = end;
    }
  normalize_glyphs_cluster (buffer, start, end, backward);
}
