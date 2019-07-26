


























#include "hb-private.hh"

#include "hb-ft.h"

#include "hb-font-private.hh"

#include FT_ADVANCES_H
#include FT_TRUETYPE_TABLES_H



#ifndef HB_DEBUG_FT
#define HB_DEBUG_FT (HB_DEBUG+0)
#endif



























static hb_bool_t
hb_ft_get_glyph (hb_font_t *font HB_UNUSED,
		 void *font_data,
		 hb_codepoint_t unicode,
		 hb_codepoint_t variation_selector,
		 hb_codepoint_t *glyph,
		 void *user_data HB_UNUSED)

{
  FT_Face ft_face = (FT_Face) font_data;

#ifdef HAVE_FT_FACE_GETCHARVARIANTINDEX
  if (unlikely (variation_selector)) {
    *glyph = FT_Face_GetCharVariantIndex (ft_face, unicode, variation_selector);
    if (*glyph)
      return true;
  }
#endif

  *glyph = FT_Get_Char_Index (ft_face, unicode);
  return *glyph != 0;
}

static hb_position_t
hb_ft_get_glyph_h_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING;
  FT_Fixed v;

  if (unlikely (FT_Get_Advance (ft_face, glyph, load_flags, &v)))
    return 0;

  return v >> 10;
}

static hb_position_t
hb_ft_get_glyph_v_advance (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   hb_codepoint_t glyph,
			   void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_VERTICAL_LAYOUT;
  FT_Fixed v;

  if (unlikely (FT_Get_Advance (ft_face, glyph, load_flags, &v)))
    return 0;

  

  return -v >> 10;
}

static hb_bool_t
hb_ft_get_glyph_h_origin (hb_font_t *font HB_UNUSED,
			  void *font_data HB_UNUSED,
			  hb_codepoint_t glyph HB_UNUSED,
			  hb_position_t *x HB_UNUSED,
			  hb_position_t *y HB_UNUSED,
			  void *user_data HB_UNUSED)
{
  
  return true;
}

static hb_bool_t
hb_ft_get_glyph_v_origin (hb_font_t *font HB_UNUSED,
			  void *font_data,
			  hb_codepoint_t glyph,
			  hb_position_t *x,
			  hb_position_t *y,
			  void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT;

  if (unlikely (FT_Load_Glyph (ft_face, glyph, load_flags)))
    return false;

  

  *x = ft_face->glyph->metrics.horiBearingX -   ft_face->glyph->metrics.vertBearingX;
  *y = ft_face->glyph->metrics.horiBearingY - (-ft_face->glyph->metrics.vertBearingY);

  return true;
}

static hb_position_t
hb_ft_get_glyph_h_kerning (hb_font_t *font,
			   void *font_data,
			   hb_codepoint_t left_glyph,
			   hb_codepoint_t right_glyph,
			   void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  FT_Vector kerningv;

  FT_Kerning_Mode mode = font->x_ppem ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED;
  if (FT_Get_Kerning (ft_face, left_glyph, right_glyph, mode, &kerningv))
    return 0;

  return kerningv.x;
}

static hb_position_t
hb_ft_get_glyph_v_kerning (hb_font_t *font HB_UNUSED,
			   void *font_data HB_UNUSED,
			   hb_codepoint_t top_glyph HB_UNUSED,
			   hb_codepoint_t bottom_glyph HB_UNUSED,
			   void *user_data HB_UNUSED)
{
  
  return 0;
}

static hb_bool_t
hb_ft_get_glyph_extents (hb_font_t *font HB_UNUSED,
			 void *font_data,
			 hb_codepoint_t glyph,
			 hb_glyph_extents_t *extents,
			 void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT;

  if (unlikely (FT_Load_Glyph (ft_face, glyph, load_flags)))
    return false;

  extents->x_bearing = ft_face->glyph->metrics.horiBearingX;
  extents->y_bearing = ft_face->glyph->metrics.horiBearingY;
  extents->width = ft_face->glyph->metrics.width;
  extents->height = -ft_face->glyph->metrics.height;
  return true;
}

static hb_bool_t
hb_ft_get_glyph_contour_point (hb_font_t *font HB_UNUSED,
			       void *font_data,
			       hb_codepoint_t glyph,
			       unsigned int point_index,
			       hb_position_t *x,
			       hb_position_t *y,
			       void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT;

  if (unlikely (FT_Load_Glyph (ft_face, glyph, load_flags)))
      return false;

  if (unlikely (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE))
      return false;

  if (unlikely (point_index >= (unsigned int) ft_face->glyph->outline.n_points))
      return false;

  *x = ft_face->glyph->outline.points[point_index].x;
  *y = ft_face->glyph->outline.points[point_index].y;

  return true;
}

static hb_bool_t
hb_ft_get_glyph_name (hb_font_t *font HB_UNUSED,
		      void *font_data,
		      hb_codepoint_t glyph,
		      char *name, unsigned int size,
		      void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;

  hb_bool_t ret = !FT_Get_Glyph_Name (ft_face, glyph, name, size);
  if (!ret || (size && !*name))
    snprintf (name, size, "gid%u", glyph);

  return ret;
}

static hb_bool_t
hb_ft_get_glyph_from_name (hb_font_t *font HB_UNUSED,
			   void *font_data,
			   const char *name, int len, 
			   hb_codepoint_t *glyph,
			   void *user_data HB_UNUSED)
{
  FT_Face ft_face = (FT_Face) font_data;

  if (len < 0)
    *glyph = FT_Get_Name_Index (ft_face, (FT_String *) name);
  else {
    
    char buf[128];
    len = MIN (len, (int) sizeof (buf) - 1);
    strncpy (buf, name, len);
    buf[len] = '\0';
    *glyph = FT_Get_Name_Index (ft_face, buf);
  }

  return *glyph != 0;
}


static hb_font_funcs_t *
_hb_ft_get_font_funcs (void)
{
  static const hb_font_funcs_t ft_ffuncs = {
    HB_OBJECT_HEADER_STATIC,

    true, 

    {
#define HB_FONT_FUNC_IMPLEMENT(name) hb_ft_get_##name,
      HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
    }
  };

  return const_cast<hb_font_funcs_t *> (&ft_ffuncs);
}


static hb_blob_t *
reference_table  (hb_face_t *face HB_UNUSED, hb_tag_t tag, void *user_data)
{
  FT_Face ft_face = (FT_Face) user_data;
  FT_Byte *buffer;
  FT_ULong  length = 0;
  FT_Error error;

  

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, NULL, &length);
  if (error)
    return NULL;

  buffer = (FT_Byte *) malloc (length);
  if (buffer == NULL)
    return NULL;

  error = FT_Load_Sfnt_Table (ft_face, tag, 0, buffer, &length);
  if (error)
    return NULL;

  return hb_blob_create ((const char *) buffer, length,
			 HB_MEMORY_MODE_WRITABLE,
			 buffer, free);
}


hb_face_t *
hb_ft_face_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy)
{
  hb_face_t *face;

  if (ft_face->stream->read == NULL) {
    hb_blob_t *blob;

    blob = hb_blob_create ((const char *) ft_face->stream->base,
			   (unsigned int) ft_face->stream->size,
			   



			   HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
			   ft_face, destroy);
    face = hb_face_create (blob, ft_face->face_index);
    hb_blob_destroy (blob);
  } else {
    face = hb_face_create_for_tables (reference_table, ft_face, destroy);
  }

  hb_face_set_index (face, ft_face->face_index);
  hb_face_set_upem (face, ft_face->units_per_EM);

  return face;
}

static void
hb_ft_face_finalize (FT_Face ft_face)
{
  hb_face_destroy ((hb_face_t *) ft_face->generic.data);
}

hb_face_t *
hb_ft_face_create_cached (FT_Face ft_face)
{
  if (unlikely (!ft_face->generic.data || ft_face->generic.finalizer != (FT_Generic_Finalizer) hb_ft_face_finalize))
  {
    if (ft_face->generic.finalizer)
      ft_face->generic.finalizer (ft_face);

    ft_face->generic.data = hb_ft_face_create (ft_face, NULL);
    ft_face->generic.finalizer = (FT_Generic_Finalizer) hb_ft_face_finalize;
  }

  return hb_face_reference ((hb_face_t *) ft_face->generic.data);
}

static void
_do_nothing (void)
{
}


hb_font_t *
hb_ft_font_create (FT_Face           ft_face,
		   hb_destroy_func_t destroy)
{
  hb_font_t *font;
  hb_face_t *face;

  face = hb_ft_face_create (ft_face, destroy);
  font = hb_font_create (face);
  hb_face_destroy (face);
  hb_font_set_funcs (font,
		     _hb_ft_get_font_funcs (),
		     ft_face, (hb_destroy_func_t) _do_nothing);
  hb_font_set_scale (font,
		     ((uint64_t) ft_face->size->metrics.x_scale * (uint64_t) ft_face->units_per_EM) >> 16,
		     ((uint64_t) ft_face->size->metrics.y_scale * (uint64_t) ft_face->units_per_EM) >> 16);
  hb_font_set_ppem (font,
		    ft_face->size->metrics.x_ppem,
		    ft_face->size->metrics.y_ppem);

  return font;
}




static FT_Library ft_library;

static inline
void free_ft_library (void)
{
  FT_Done_FreeType (ft_library);
}

static FT_Library
get_ft_library (void)
{
retry:
  FT_Library library = (FT_Library) hb_atomic_ptr_get (&ft_library);

  if (unlikely (!library))
  {
    
    if (FT_Init_FreeType (&library))
      return NULL;

    if (!hb_atomic_ptr_cmpexch (&ft_library, NULL, library)) {
      FT_Done_FreeType (library);
      goto retry;
    }

#ifdef HAVE_ATEXIT
    atexit (free_ft_library); 
#endif
  }

  return library;
}

static void
_release_blob (FT_Face ft_face)
{
  hb_blob_destroy ((hb_blob_t *) ft_face->generic.data);
}

void
hb_ft_font_set_funcs (hb_font_t *font)
{
  hb_blob_t *blob = hb_face_reference_blob (font->face);
  unsigned int blob_length;
  const char *blob_data = hb_blob_get_data (blob, &blob_length);
  if (unlikely (!blob_length))
    DEBUG_MSG (FT, font, "Font face has empty blob");

  FT_Face ft_face = NULL;
  FT_Error err = FT_New_Memory_Face (get_ft_library (),
				     (const FT_Byte *) blob_data,
				     blob_length,
				     hb_face_get_index (font->face),
				     &ft_face);

  if (unlikely (err)) {
    hb_blob_destroy (blob);
    DEBUG_MSG (FT, font, "Font face FT_New_Memory_Face() failed");
    return;
  }

  FT_Select_Charmap (ft_face, FT_ENCODING_UNICODE);

  assert (font->y_scale >= 0);
  FT_Set_Char_Size (ft_face,
		    font->x_scale, font->y_scale,
		    0, 0);
#if 0
		    font->x_ppem * 72 * 64 / font->x_scale,
		    font->y_ppem * 72 * 64 / font->y_scale);
#endif

  ft_face->generic.data = blob;
  ft_face->generic.finalizer = (FT_Generic_Finalizer) _release_blob;

  hb_font_set_funcs (font,
		     _hb_ft_get_font_funcs (),
		     ft_face,
		     (hb_destroy_func_t) FT_Done_Face);
}

FT_Face
hb_ft_font_get_face (hb_font_t *font)
{
  if (font->destroy == (hb_destroy_func_t) FT_Done_Face ||
      font->destroy == (hb_destroy_func_t) _do_nothing)
    return (FT_Face) font->user_data;

  return NULL;
}
