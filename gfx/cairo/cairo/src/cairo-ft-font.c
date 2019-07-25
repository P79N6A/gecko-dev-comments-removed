







































#define _BSD_SOURCE
#include "cairoint.h"

#include "cairo-error-private.h"
#include "cairo-ft-private.h"

#include <float.h>

#include "cairo-fontconfig-private.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_IMAGE_H
#include FT_TRUETYPE_TABLES_H
#if HAVE_FT_GLYPHSLOT_EMBOLDEN
#include FT_SYNTHESIS_H
#endif

#if HAVE_FT_LIBRARY_SETLCDFILTER
#include FT_LCD_FILTER_H
#endif

#define _GNU_SOURCE
#include <dlfcn.h>

#ifndef RTLD_DEFAULT
#define RTLD_DEFAULT ((void *) 0)
#endif


#ifndef FC_LCD_FILTER
#define FC_LCD_FILTER	"lcdfilter"
#endif

#ifndef FC_LCD_NONE
#define FC_LCD_NONE	0
#define FC_LCD_DEFAULT	1
#define FC_LCD_LIGHT	2
#define FC_LCD_LEGACY	3
#endif


#ifndef FT_LCD_FILTER_NONE
#define FT_LCD_FILTER_NONE	0
#define FT_LCD_FILTER_DEFAULT	1
#define FT_LCD_FILTER_LIGHT	2
#define FT_LCD_FILTER_LEGACY	16
#endif

typedef FT_Error (*setLcdFilterFunc)(FT_Library, int);
static setLcdFilterFunc setLcdFilter;

#define DOUBLE_TO_26_6(d) ((FT_F26Dot6)((d) * 64.0))
#define DOUBLE_FROM_26_6(t) ((double)(t) / 64.0)
#define DOUBLE_TO_16_16(d) ((FT_Fixed)((d) * 65536.0))
#define DOUBLE_FROM_16_16(t) ((double)(t) / 65536.0)



#define MAX_OPEN_FACES 10


#define MAX_FONT_SIZE 1000































typedef struct _cairo_ft_font_transform {
    double  x_scale, y_scale;
    double  shape[2][2];
} cairo_ft_font_transform_t;








typedef struct _cairo_ft_font_face cairo_ft_font_face_t;

struct _cairo_ft_unscaled_font {
    cairo_unscaled_font_t base;

    cairo_bool_t from_face; 
    FT_Face face;	    

    
    char *filename;
    int id;

    
    cairo_bool_t have_scale;
    cairo_matrix_t current_scale;
    double x_scale;		
    double y_scale;             
    cairo_bool_t have_shape;	
    cairo_matrix_t current_shape;
    FT_Matrix Current_Shape;

    cairo_mutex_t mutex;
    int lock_count;

    cairo_ft_font_face_t *faces;	
};

static int
_cairo_ft_unscaled_font_keys_equal (const void *key_a,
				    const void *key_b);

static void
_cairo_ft_unscaled_font_fini (cairo_ft_unscaled_font_t *unscaled);

typedef enum _cairo_ft_extra_flags {
    CAIRO_FT_OPTIONS_HINT_METRICS = (1 << 0),
    CAIRO_FT_OPTIONS_EMBOLDEN = (1 << 1)
} cairo_ft_extra_flags_t;

typedef struct _cairo_ft_options {
    cairo_font_options_t    base;
    int			    load_flags;	 
    cairo_ft_extra_flags_t  extra_flags; 
} cairo_ft_options_t;

struct _cairo_ft_font_face {
    cairo_font_face_t base;

    cairo_ft_unscaled_font_t *unscaled;
    cairo_ft_options_t ft_options;
    cairo_ft_font_face_t *next;

#if CAIRO_HAS_FC_FONT
    FcPattern *pattern; 
    cairo_font_face_t *resolved_font_face;
    FcConfig *resolved_config;
#endif
};

static const cairo_unscaled_font_backend_t cairo_ft_unscaled_font_backend;

#if CAIRO_HAS_FC_FONT
static cairo_status_t
_cairo_ft_font_options_substitute (const cairo_font_options_t *options,
				   FcPattern                  *pattern);

static cairo_font_face_t *
_cairo_ft_resolve_pattern (FcPattern		      *pattern,
			   const cairo_matrix_t       *font_matrix,
			   const cairo_matrix_t       *ctm,
			   const cairo_font_options_t *options);

#endif










typedef struct _cairo_ft_unscaled_font_map {
    cairo_hash_table_t *hash_table;
    FT_Library ft_library;
    int num_open_faces;
} cairo_ft_unscaled_font_map_t;

static cairo_ft_unscaled_font_map_t *cairo_ft_unscaled_font_map = NULL;


static void
_font_map_release_face_lock_held (cairo_ft_unscaled_font_map_t *font_map,
				  cairo_ft_unscaled_font_t *unscaled)
{
    if (unscaled->face) {
	FT_Done_Face (unscaled->face);
	unscaled->face = NULL;
	unscaled->have_scale = FALSE;

	font_map->num_open_faces--;
    }
}

static cairo_status_t
_cairo_ft_unscaled_font_map_create (void)
{
    cairo_ft_unscaled_font_map_t *font_map;

    


    assert (cairo_ft_unscaled_font_map == NULL);

    font_map = malloc (sizeof (cairo_ft_unscaled_font_map_t));
    if (unlikely (font_map == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font_map->hash_table =
	_cairo_hash_table_create (_cairo_ft_unscaled_font_keys_equal);

    if (unlikely (font_map->hash_table == NULL))
	goto FAIL;

    if (unlikely (FT_Init_FreeType (&font_map->ft_library)))
	goto FAIL;

    font_map->num_open_faces = 0;

    cairo_ft_unscaled_font_map = font_map;
    return CAIRO_STATUS_SUCCESS;

FAIL:
    if (font_map->hash_table)
	_cairo_hash_table_destroy (font_map->hash_table);
    free (font_map);

    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
}


static void
_cairo_ft_unscaled_font_map_pluck_entry (void *entry, void *closure)
{
    cairo_ft_unscaled_font_t *unscaled = entry;
    cairo_ft_unscaled_font_map_t *font_map = closure;

    _cairo_hash_table_remove (font_map->hash_table,
			      &unscaled->base.hash_entry);

    if (! unscaled->from_face)
	_font_map_release_face_lock_held (font_map, unscaled);

    _cairo_ft_unscaled_font_fini (unscaled);
    free (unscaled);
}

static void
_cairo_ft_unscaled_font_map_destroy (void)
{
    cairo_ft_unscaled_font_map_t *font_map;

    CAIRO_MUTEX_LOCK (_cairo_ft_unscaled_font_map_mutex);
    font_map = cairo_ft_unscaled_font_map;
    cairo_ft_unscaled_font_map = NULL;
    CAIRO_MUTEX_UNLOCK (_cairo_ft_unscaled_font_map_mutex);

    if (font_map != NULL) {
	_cairo_hash_table_foreach (font_map->hash_table,
				   _cairo_ft_unscaled_font_map_pluck_entry,
				   font_map);
	assert (font_map->num_open_faces == 0);

	FT_Done_FreeType (font_map->ft_library);

	_cairo_hash_table_destroy (font_map->hash_table);

	free (font_map);
    }
}

static cairo_ft_unscaled_font_map_t *
_cairo_ft_unscaled_font_map_lock (void)
{
    CAIRO_MUTEX_LOCK (_cairo_ft_unscaled_font_map_mutex);

    if (unlikely (cairo_ft_unscaled_font_map == NULL)) {
	if (unlikely (_cairo_ft_unscaled_font_map_create ())) {
	    CAIRO_MUTEX_UNLOCK (_cairo_ft_unscaled_font_map_mutex);
	    return NULL;
	}
    }

    return cairo_ft_unscaled_font_map;
}

static void
_cairo_ft_unscaled_font_map_unlock (void)
{
    CAIRO_MUTEX_UNLOCK (_cairo_ft_unscaled_font_map_mutex);
}

static void
_cairo_ft_unscaled_font_init_key (cairo_ft_unscaled_font_t *key,
				  cairo_bool_t              from_face,
				  char			   *filename,
				  int			    id,
				  FT_Face		    face)
{
    unsigned long hash;

    key->from_face = from_face;
    key->filename = filename;
    key->id = id;
    key->face = face;

    hash = _cairo_hash_string (filename);
    
    hash += ((unsigned long) id) * 1607;
    hash += ((unsigned long) face) * 2137;

    key->base.hash_entry.hash = hash;
}






















static cairo_status_t
_cairo_ft_unscaled_font_init (cairo_ft_unscaled_font_t *unscaled,
			      cairo_bool_t              from_face,
			      const char	       *filename,
			      int			id,
			      FT_Face			face)
{
    _cairo_unscaled_font_init (&unscaled->base,
			       &cairo_ft_unscaled_font_backend);

    if (from_face) {
	unscaled->from_face = TRUE;
	_cairo_ft_unscaled_font_init_key (unscaled, TRUE, NULL, 0, face);
    } else {
	char *filename_copy;

	unscaled->from_face = FALSE;
	unscaled->face = NULL;

	filename_copy = strdup (filename);
	if (unlikely (filename_copy == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_cairo_ft_unscaled_font_init_key (unscaled, FALSE, filename_copy, id, NULL);
    }

    unscaled->have_scale = FALSE;
    CAIRO_MUTEX_INIT (unscaled->mutex);
    unscaled->lock_count = 0;

    unscaled->faces = NULL;

    return CAIRO_STATUS_SUCCESS;
}












static void
_cairo_ft_unscaled_font_fini (cairo_ft_unscaled_font_t *unscaled)
{
    assert (unscaled->face == NULL);

    if (unscaled->filename) {
	free (unscaled->filename);
	unscaled->filename = NULL;
    }

    CAIRO_MUTEX_FINI (unscaled->mutex);
}

static int
_cairo_ft_unscaled_font_keys_equal (const void *key_a,
				    const void *key_b)
{
    const cairo_ft_unscaled_font_t *unscaled_a = key_a;
    const cairo_ft_unscaled_font_t *unscaled_b = key_b;

    if (unscaled_a->id == unscaled_b->id &&
	unscaled_a->from_face == unscaled_b->from_face)
    {
        if (unscaled_a->from_face)
	    return unscaled_a->face == unscaled_b->face;

	if (unscaled_a->filename == NULL && unscaled_b->filename == NULL)
	    return TRUE;
	else if (unscaled_a->filename == NULL || unscaled_b->filename == NULL)
	    return FALSE;
	else
	    return (strcmp (unscaled_a->filename, unscaled_b->filename) == 0);
    }

    return FALSE;
}




static cairo_status_t
_cairo_ft_unscaled_font_create_internal (cairo_bool_t from_face,
					 char *filename,
					 int id,
					 FT_Face font_face,
					 cairo_ft_unscaled_font_t **out)
{
    cairo_ft_unscaled_font_t key, *unscaled;
    cairo_ft_unscaled_font_map_t *font_map;
    cairo_status_t status;

    font_map = _cairo_ft_unscaled_font_map_lock ();
    if (unlikely (font_map == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    _cairo_ft_unscaled_font_init_key (&key, from_face, filename, id, font_face);

    
    unscaled = _cairo_hash_table_lookup (font_map->hash_table,
					 &key.base.hash_entry);
    if (unscaled != NULL) {
	_cairo_unscaled_font_reference (&unscaled->base);
	goto DONE;
    }

    
    unscaled = malloc (sizeof (cairo_ft_unscaled_font_t));
    if (unlikely (unscaled == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto UNWIND_FONT_MAP_LOCK;
    }

    status = _cairo_ft_unscaled_font_init (unscaled, from_face, filename, id, font_face);
    if (unlikely (status))
	goto UNWIND_UNSCALED_MALLOC;

    assert (unscaled->base.hash_entry.hash == key.base.hash_entry.hash);
    status = _cairo_hash_table_insert (font_map->hash_table,
				       &unscaled->base.hash_entry);
    if (unlikely (status))
	goto UNWIND_UNSCALED_FONT_INIT;

DONE:
    _cairo_ft_unscaled_font_map_unlock ();
    *out = unscaled;
    return CAIRO_STATUS_SUCCESS;

UNWIND_UNSCALED_FONT_INIT:
    _cairo_ft_unscaled_font_fini (unscaled);
UNWIND_UNSCALED_MALLOC:
    free (unscaled);
UNWIND_FONT_MAP_LOCK:
    _cairo_ft_unscaled_font_map_unlock ();
    return status;
}


#if CAIRO_HAS_FC_FONT
static cairo_status_t
_cairo_ft_unscaled_font_create_for_pattern (FcPattern *pattern,
					    cairo_ft_unscaled_font_t **out)
{
    FT_Face font_face = NULL;
    char *filename = NULL;
    int id = 0;
    FcResult ret;

    ret = FcPatternGetFTFace (pattern, FC_FT_FACE, 0, &font_face);
    if (ret == FcResultMatch)
	goto DONE;
    if (ret == FcResultOutOfMemory)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    ret = FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **) &filename);
    if (ret == FcResultOutOfMemory)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    if (ret == FcResultMatch) {
	
	ret = FcPatternGetInteger (pattern, FC_INDEX, 0, &id);
	if (ret == FcResultOutOfMemory)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	goto DONE;
    }

    
    *out = NULL;
    return CAIRO_STATUS_SUCCESS;

DONE:
    return _cairo_ft_unscaled_font_create_internal (font_face != NULL,
						    filename, id, font_face,
						    out);
}
#endif

static cairo_status_t
_cairo_ft_unscaled_font_create_from_face (FT_Face face,
					  cairo_ft_unscaled_font_t **out)
{
    return _cairo_ft_unscaled_font_create_internal (TRUE, NULL, 0, face, out);
}

static void
_cairo_ft_unscaled_font_destroy (void *abstract_font)
{
    cairo_ft_unscaled_font_t *unscaled  = abstract_font;
    cairo_ft_unscaled_font_map_t *font_map;

    if (unscaled == NULL)
	return;

    font_map = _cairo_ft_unscaled_font_map_lock ();
    
    assert (font_map != NULL);

    if (CAIRO_REFERENCE_COUNT_HAS_REFERENCE (&unscaled->base.ref_count)) {
	
	_cairo_ft_unscaled_font_map_unlock ();
	return;
    }

    _cairo_hash_table_remove (font_map->hash_table,
			      &unscaled->base.hash_entry);

    if (unscaled->from_face) {
	


	if (unscaled->faces && unscaled->faces->unscaled == NULL) {
	    assert (unscaled->faces->next == NULL);
	    cairo_font_face_destroy (&unscaled->faces->base);
	}
    } else {
	_font_map_release_face_lock_held (font_map, unscaled);
    }
    unscaled->face = NULL;

    _cairo_ft_unscaled_font_map_unlock ();

    _cairo_ft_unscaled_font_fini (unscaled);
}

static cairo_bool_t
_has_unlocked_face (const void *entry)
{
    const cairo_ft_unscaled_font_t *unscaled = entry;

    return (!unscaled->from_face && unscaled->lock_count == 0 && unscaled->face);
}







cairo_warn FT_Face
_cairo_ft_unscaled_font_lock_face (cairo_ft_unscaled_font_t *unscaled)
{
    cairo_ft_unscaled_font_map_t *font_map;
    FT_Face face = NULL;

    CAIRO_MUTEX_LOCK (unscaled->mutex);
    unscaled->lock_count++;

    if (unscaled->face)
	return unscaled->face;

    

    assert (!unscaled->from_face);

    font_map = _cairo_ft_unscaled_font_map_lock ();
    {
	assert (font_map != NULL);

	while (font_map->num_open_faces >= MAX_OPEN_FACES)
	{
	    cairo_ft_unscaled_font_t *entry;

	    entry = _cairo_hash_table_random_entry (font_map->hash_table,
						    _has_unlocked_face);
	    if (entry == NULL)
		break;

	    _font_map_release_face_lock_held (font_map, entry);
	}
    }
    _cairo_ft_unscaled_font_map_unlock ();

    if (FT_New_Face (font_map->ft_library,
		     unscaled->filename,
		     unscaled->id,
		     &face) != FT_Err_Ok)
    {
	unscaled->lock_count--;
	CAIRO_MUTEX_UNLOCK (unscaled->mutex);
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    unscaled->face = face;

    font_map->num_open_faces++;

    return face;
}




void
_cairo_ft_unscaled_font_unlock_face (cairo_ft_unscaled_font_t *unscaled)
{
    assert (unscaled->lock_count > 0);

    unscaled->lock_count--;

    CAIRO_MUTEX_UNLOCK (unscaled->mutex);
}


static cairo_status_t
_compute_transform (cairo_ft_font_transform_t *sf,
		    cairo_matrix_t      *scale)
{
    cairo_status_t status;
    double x_scale, y_scale;
    cairo_matrix_t normalized = *scale;

    






    status = _cairo_matrix_compute_basis_scale_factors (scale,
						  &x_scale, &y_scale,
						  1);
    if (unlikely (status))
	return status;

    



    if (x_scale < 1.0)
      x_scale = 1.0;
    if (y_scale < 1.0)
      y_scale = 1.0;

    sf->x_scale = x_scale;
    sf->y_scale = y_scale;

    cairo_matrix_scale (&normalized, 1.0 / x_scale, 1.0 / y_scale);

    _cairo_matrix_get_affine (&normalized,
			      &sf->shape[0][0], &sf->shape[0][1],
			      &sf->shape[1][0], &sf->shape[1][1],
			      NULL, NULL);

    return CAIRO_STATUS_SUCCESS;
}




static cairo_status_t
_cairo_ft_unscaled_font_set_scale (cairo_ft_unscaled_font_t *unscaled,
				   cairo_matrix_t	      *scale)
{
    cairo_status_t status;
    cairo_ft_font_transform_t sf;
    FT_Matrix mat;
    FT_Error error;

    assert (unscaled->face != NULL);

    if (unscaled->have_scale &&
	scale->xx == unscaled->current_scale.xx &&
	scale->yx == unscaled->current_scale.yx &&
	scale->xy == unscaled->current_scale.xy &&
	scale->yy == unscaled->current_scale.yy)
	return CAIRO_STATUS_SUCCESS;

    unscaled->have_scale = TRUE;
    unscaled->current_scale = *scale;

    status = _compute_transform (&sf, scale);
    if (unlikely (status))
	return status;

    unscaled->x_scale = sf.x_scale;
    unscaled->y_scale = sf.y_scale;

    mat.xx = DOUBLE_TO_16_16(sf.shape[0][0]);
    mat.yx = - DOUBLE_TO_16_16(sf.shape[0][1]);
    mat.xy = - DOUBLE_TO_16_16(sf.shape[1][0]);
    mat.yy = DOUBLE_TO_16_16(sf.shape[1][1]);

    unscaled->have_shape = (mat.xx != 0x10000 ||
			    mat.yx != 0x00000 ||
			    mat.xy != 0x00000 ||
			    mat.yy != 0x10000);

    unscaled->Current_Shape = mat;
    cairo_matrix_init (&unscaled->current_shape,
		       sf.shape[0][0], sf.shape[0][1],
		       sf.shape[1][0], sf.shape[1][1],
		       0.0, 0.0);

    FT_Set_Transform(unscaled->face, &mat, NULL);

    if ((unscaled->face->face_flags & FT_FACE_FLAG_SCALABLE) != 0) {
	double x_scale = MIN(sf.x_scale, MAX_FONT_SIZE);
	double y_scale = MIN(sf.y_scale, MAX_FONT_SIZE);
	error = FT_Set_Char_Size (unscaled->face,
				  x_scale * 64.0 + .5,
				  y_scale * 64.0 + .5,
				  0, 0);
	if (error)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    } else {
	double min_distance = DBL_MAX;
	int i;
	int best_i = 0;

	for (i = 0; i < unscaled->face->num_fixed_sizes; i++) {
#if HAVE_FT_BITMAP_SIZE_Y_PPEM
	    double size = unscaled->face->available_sizes[i].y_ppem / 64.;
#else
	    double size = unscaled->face->available_sizes[i].height;
#endif
	    double distance = fabs (size - sf.y_scale);

	    if (distance <= min_distance) {
		min_distance = distance;
		best_i = i;
	    }
	}
#if HAVE_FT_BITMAP_SIZE_Y_PPEM
	error = FT_Set_Char_Size (unscaled->face,
				  unscaled->face->available_sizes[best_i].x_ppem,
				  unscaled->face->available_sizes[best_i].y_ppem,
				  0, 0);
	if (error)
#endif
	    error = FT_Set_Pixel_Sizes (unscaled->face,
					unscaled->face->available_sizes[best_i].width,
					unscaled->face->available_sizes[best_i].height);
	if (error)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    return CAIRO_STATUS_SUCCESS;
}
























static int
_compute_xrender_bitmap_size(FT_Bitmap      *target,
			     FT_GlyphSlot    slot,
			     FT_Render_Mode  mode)
{
    FT_Bitmap *ftbit;
    int width, height, pitch;

    if (slot->format != FT_GLYPH_FORMAT_BITMAP)
	return -1;

    
    ftbit = &slot->bitmap;

    width = ftbit->width;
    height = ftbit->rows;
    pitch = (width + 3) & ~3;

    switch (ftbit->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	if (mode == FT_RENDER_MODE_MONO) {
	    pitch = (((width + 31) & ~31) >> 3);
	    break;
	}
	

    case FT_PIXEL_MODE_GRAY:
	if (mode == FT_RENDER_MODE_LCD ||
	    mode == FT_RENDER_MODE_LCD_V)
	{
	    
	    pitch = width * 4;
	}
	break;

    case FT_PIXEL_MODE_LCD:
	if (mode != FT_RENDER_MODE_LCD)
	    return -1;

	
	width /= 3;
	pitch = width * 4;
	break;

    case FT_PIXEL_MODE_LCD_V:
	if (mode != FT_RENDER_MODE_LCD_V)
	    return -1;

	
	height /= 3;
	pitch = width * 4;
	break;

    default:  
	return -1;
    }

    target->width = width;
    target->rows = height;
    target->pitch = pitch;
    target->buffer = NULL;

    return pitch * height;
}















static void
_fill_xrender_bitmap(FT_Bitmap      *target,
		     FT_GlyphSlot    slot,
		     FT_Render_Mode  mode,
		     int             bgr)
{
    FT_Bitmap *ftbit = &slot->bitmap;
    unsigned char *srcLine = ftbit->buffer;
    unsigned char *dstLine = target->buffer;
    int src_pitch = ftbit->pitch;
    int width = target->width;
    int height = target->rows;
    int pitch = target->pitch;
    int subpixel;
    int h;

    subpixel = (mode == FT_RENDER_MODE_LCD ||
		mode == FT_RENDER_MODE_LCD_V);

    if (src_pitch < 0)
	srcLine -= src_pitch * (ftbit->rows - 1);

    target->pixel_mode = ftbit->pixel_mode;

    switch (ftbit->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	if (subpixel) {
	    

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;

		for (x = 0; x < width; x++) {
		    if (srcLine[(x >> 3)] & (0x80 >> (x & 7)))
			((unsigned int *) dstLine)[x] = 0xffffffffU;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_LCD;

	} else if (mode == FT_RENDER_MODE_NORMAL) {
	    

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;

		for (x = 0; x < width; x++) {
		    if (srcLine[(x >> 3)] & (0x80 >> (x & 7)))
			dstLine[x] = 0xff;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_GRAY;

	} else {
	    

	    int  bytes = (width + 7) >> 3;

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch)
		memcpy (dstLine, srcLine, bytes);
	}
	break;

    case FT_PIXEL_MODE_GRAY:
	if (subpixel) {
	    

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++) {
		    unsigned int pix = srcLine[x];

		    pix |= (pix << 8);
		    pix |= (pix << 16);

		    dst[x] = pix;
		}
	    }
	    target->pixel_mode = FT_PIXEL_MODE_LCD;
        } else {
            

            for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch)
                memcpy (dstLine, srcLine, width);
        }
        break;

    case FT_PIXEL_MODE_LCD:
	if (!bgr) {
	    

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {
		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 3) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[0] << 16) |
			  ((unsigned int)src[1] <<  8) |
			  ((unsigned int)src[2]      ) |
			  ((unsigned int)src[1] << 24) ;

		    dst[x] = pix;
		}
	    }
	} else {
	    

	    for (h = height; h > 0; h--, srcLine += src_pitch, dstLine += pitch) {

		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 3) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[2] << 16) |
			  ((unsigned int)src[1] <<  8) |
			  ((unsigned int)src[0]      ) |
			  ((unsigned int)src[1] << 24) ;

		    dst[x] = pix;
		}
	    }
	}
	break;

    default:  
	
	if (!bgr) {

	    for (h = height; h > 0; h--, srcLine += 3 * src_pitch, dstLine += pitch) {
		int x;
		unsigned char* src = srcLine;
		unsigned int*  dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 1) {
		    unsigned int pix;
		    pix = ((unsigned int)src[0]           << 16) |
			  ((unsigned int)src[src_pitch]   <<  8) |
			  ((unsigned int)src[src_pitch*2]      ) |
			  ((unsigned int)src[src_pitch]   << 24) ;
		    dst[x] = pix;
		}
	    }
	} else {

	    for (h = height; h > 0; h--, srcLine += 3*src_pitch, dstLine += pitch) {
		int x;
		unsigned char *src = srcLine;
		unsigned int *dst = (unsigned int *) dstLine;

		for (x = 0; x < width; x++, src += 1) {
		    unsigned int  pix;

		    pix = ((unsigned int)src[src_pitch * 2] << 16) |
			  ((unsigned int)src[src_pitch]     <<  8) |
			  ((unsigned int)src[0]                  ) |
			  ((unsigned int)src[src_pitch]     << 24) ;

		    dst[x] = pix;
		}
	    }
	}
    }
}




static cairo_status_t
_get_bitmap_surface (FT_Bitmap		     *bitmap,
		     cairo_bool_t	      own_buffer,
		     cairo_font_options_t    *font_options,
		     cairo_image_surface_t  **surface)
{
    int width, height, stride;
    unsigned char *data;
    int format = CAIRO_FORMAT_A8;
    cairo_image_surface_t *image;

    width = bitmap->width;
    height = bitmap->rows;

    if (width == 0 || height == 0) {
	*surface = (cairo_image_surface_t *)
	    cairo_image_surface_create_for_data (NULL, format, 0, 0, 0);
	return (*surface)->base.status;
    }

    switch (bitmap->pixel_mode) {
    case FT_PIXEL_MODE_MONO:
	stride = (((width + 31) & ~31) >> 3);
	if (own_buffer) {
	    data = bitmap->buffer;
	    assert (stride == bitmap->pitch);
	} else {
	    data = _cairo_malloc_ab (height, stride);
	    if (!data)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    if (stride == bitmap->pitch) {
		memcpy (data, bitmap->buffer, stride * height);
	    } else {
		int i;
		unsigned char *source, *dest;

		source = bitmap->buffer;
		dest = data;
		for (i = height; i; i--) {
		    memcpy (dest, source, bitmap->pitch);
		    memset (dest + bitmap->pitch, '\0', stride - bitmap->pitch);

		    source += bitmap->pitch;
		    dest += stride;
		}
	    }
	}

#ifndef WORDS_BIGENDIAN
	{
	    uint8_t   *d = data;
	    int		count = stride * height;

	    while (count--) {
		*d = CAIRO_BITSWAP8 (*d);
		d++;
	    }
	}
#endif
	format = CAIRO_FORMAT_A1;
	break;

    case FT_PIXEL_MODE_LCD:
    case FT_PIXEL_MODE_LCD_V:
    case FT_PIXEL_MODE_GRAY:
        if (font_options->antialias != CAIRO_ANTIALIAS_SUBPIXEL) {
	    stride = bitmap->pitch;
	    if (own_buffer) {
		data = bitmap->buffer;
	    } else {
		data = _cairo_malloc_ab (height, stride);
		if (!data)
		    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

		memcpy (data, bitmap->buffer, stride * height);
	    }

	format = CAIRO_FORMAT_A8;
	} else {
	    


	    assert (own_buffer != 0);
	    assert (bitmap->pixel_mode != FT_PIXEL_MODE_GRAY);

		data = bitmap->buffer;
		stride = bitmap->pitch;
		format = CAIRO_FORMAT_ARGB32;
	}
	break;
    case FT_PIXEL_MODE_GRAY2:
    case FT_PIXEL_MODE_GRAY4:
	
    default:
	if (own_buffer)
	    free (bitmap->buffer);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    
    *surface = image = (cairo_image_surface_t *)
	cairo_image_surface_create_for_data (data,
					     format,
					     width, height, stride);
    if (image->base.status) {
	free (data);
	return (*surface)->base.status;
    }

    if (format == CAIRO_FORMAT_ARGB32)
	pixman_image_set_component_alpha (image->pixman_image, TRUE);

    _cairo_image_surface_assume_ownership_of_data (image);

    _cairo_debug_check_image_surface_is_defined (&image->base);

    return CAIRO_STATUS_SUCCESS;
}
















static cairo_status_t
_render_glyph_outline (FT_Face                    face,
		       cairo_font_options_t	 *font_options,
		       cairo_image_surface_t	**surface)
{
    int rgba = FC_RGBA_UNKNOWN;
    int lcd_filter = FT_LCD_FILTER_LEGACY;
    FT_GlyphSlot glyphslot = face->glyph;
    FT_Outline *outline = &glyphslot->outline;
    FT_Bitmap bitmap;
    FT_BBox cbox;
    unsigned int width, height;
    cairo_status_t status;
    FT_Error fterror;
    FT_Library library = glyphslot->library;
    FT_Render_Mode render_mode = FT_RENDER_MODE_NORMAL;

    switch (font_options->antialias) {
    case CAIRO_ANTIALIAS_NONE:
	render_mode = FT_RENDER_MODE_MONO;
	break;

    case CAIRO_ANTIALIAS_SUBPIXEL:
	switch (font_options->subpixel_order) {
	    case CAIRO_SUBPIXEL_ORDER_DEFAULT:
	    case CAIRO_SUBPIXEL_ORDER_RGB:
	    case CAIRO_SUBPIXEL_ORDER_BGR:
		render_mode = FT_RENDER_MODE_LCD;
		break;

	    case CAIRO_SUBPIXEL_ORDER_VRGB:
	    case CAIRO_SUBPIXEL_ORDER_VBGR:
		render_mode = FT_RENDER_MODE_LCD_V;
		break;
	}

	switch (font_options->lcd_filter) {
	case CAIRO_LCD_FILTER_NONE:
	    lcd_filter = FT_LCD_FILTER_NONE;
	    break;
	case CAIRO_LCD_FILTER_DEFAULT:
	case CAIRO_LCD_FILTER_INTRA_PIXEL:
	    lcd_filter = FT_LCD_FILTER_LEGACY;
	    break;
	case CAIRO_LCD_FILTER_FIR3:
	    lcd_filter = FT_LCD_FILTER_LIGHT;
	    break;
	case CAIRO_LCD_FILTER_FIR5:
	    lcd_filter = FT_LCD_FILTER_DEFAULT;
	    break;
	}

	break;

    case CAIRO_ANTIALIAS_DEFAULT:
    case CAIRO_ANTIALIAS_GRAY:
	render_mode = FT_RENDER_MODE_NORMAL;
    }

    FT_Outline_Get_CBox (outline, &cbox);

    cbox.xMin &= -64;
    cbox.yMin &= -64;
    cbox.xMax = (cbox.xMax + 63) & -64;
    cbox.yMax = (cbox.yMax + 63) & -64;

    width = (unsigned int) ((cbox.xMax - cbox.xMin) >> 6);
    height = (unsigned int) ((cbox.yMax - cbox.yMin) >> 6);

    if (width * height == 0) {
	cairo_format_t format;
	
	switch (render_mode) {
	case FT_RENDER_MODE_MONO:
	    format = CAIRO_FORMAT_A1;
	    break;
	case FT_RENDER_MODE_LCD:
	case FT_RENDER_MODE_LCD_V:
	    format= CAIRO_FORMAT_ARGB32;
	    break;
	case FT_RENDER_MODE_LIGHT:
	case FT_RENDER_MODE_NORMAL:
	case FT_RENDER_MODE_MAX:
	default:
	    format = CAIRO_FORMAT_A8;
	    break;
	}

	(*surface) = (cairo_image_surface_t *)
	    cairo_image_surface_create_for_data (NULL, format, 0, 0, 0);
	if ((*surface)->base.status)
	    return (*surface)->base.status;
    } else {

	int bitmap_size;
        static int initialized_setLcdFilter = 0;

	switch (render_mode) {
	case FT_RENDER_MODE_LCD:
	    if (font_options->subpixel_order == CAIRO_SUBPIXEL_ORDER_BGR) {
		rgba = FC_RGBA_BGR;
	    } else {
		rgba = FC_RGBA_RGB;
	    }
	case FT_RENDER_MODE_LCD_V:
	    if (font_options->subpixel_order == CAIRO_SUBPIXEL_ORDER_VBGR) {
		rgba = FC_RGBA_VBGR;
	    } else {
		rgba = FC_RGBA_VRGB;
		}
	    break;
	case FT_RENDER_MODE_MONO:
	case FT_RENDER_MODE_LIGHT:
	case FT_RENDER_MODE_NORMAL:
	case FT_RENDER_MODE_MAX:
	default:
	    break;
	    }

        if (!initialized_setLcdFilter) {
          initialized_setLcdFilter = 1;
          setLcdFilter = (setLcdFilterFunc) dlsym(RTLD_DEFAULT, "FT_Library_SetLcdFilter");
        }

	if (setLcdFilter)
          setLcdFilter (library, lcd_filter);

	fterror = FT_Render_Glyph (face->glyph, render_mode);

	if (setLcdFilter)
          setLcdFilter (library, FT_LCD_FILTER_NONE);

	if (fterror != 0)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	bitmap_size = _compute_xrender_bitmap_size (&bitmap,
						    face->glyph,
						    render_mode);
	if (bitmap_size < 0)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	bitmap.buffer = calloc (1, bitmap_size);
	if (bitmap.buffer == NULL)
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	_fill_xrender_bitmap (&bitmap, face->glyph, render_mode,
			      (rgba == FC_RGBA_BGR || rgba == FC_RGBA_VBGR));

	


	status = _get_bitmap_surface (&bitmap, TRUE, font_options, surface);
	if (unlikely (status))
	    return status;

	




	cairo_surface_set_device_offset (&(*surface)->base,
					 (double)-glyphslot->bitmap_left,
					 (double)+glyphslot->bitmap_top);
    }

    return CAIRO_STATUS_SUCCESS;
}


static cairo_status_t
_render_glyph_bitmap (FT_Face		      face,
		      cairo_font_options_t   *font_options,
		      cairo_image_surface_t **surface)
{
    FT_GlyphSlot glyphslot = face->glyph;
    cairo_status_t status;
    FT_Error error;

    






    error = FT_Render_Glyph (glyphslot, FT_RENDER_MODE_NORMAL);
    

    if (error == FT_Err_Out_Of_Memory)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _get_bitmap_surface (&glyphslot->bitmap,
				  FALSE, font_options,
				  surface);
    if (unlikely (status))
	return status;

    






    cairo_surface_set_device_offset (&(*surface)->base,
				     -glyphslot->bitmap_left,
				     +glyphslot->bitmap_top);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_transform_glyph_bitmap (cairo_matrix_t         * shape,
			 cairo_image_surface_t ** surface)
{
    cairo_matrix_t original_to_transformed;
    cairo_matrix_t transformed_to_original;
    cairo_image_surface_t *old_image;
    cairo_surface_t *image;
    double x[4], y[4];
    double origin_x, origin_y;
    int orig_width, orig_height;
    int i;
    int x_min, y_min, x_max, y_max;
    int width, height;
    cairo_status_t status;
    cairo_surface_pattern_t pattern;

    



    original_to_transformed = *shape;
    
    cairo_surface_get_device_offset (&(*surface)->base, &origin_x, &origin_y);
    orig_width = (*surface)->width;
    orig_height = (*surface)->height;

    cairo_matrix_translate (&original_to_transformed,
			    -origin_x, -origin_y);

    


    x[0] = 0;          y[0] = 0;
    x[1] = orig_width; y[1] = 0;
    x[2] = orig_width; y[2] = orig_height;
    x[3] = 0;          y[3] = orig_height;

    for (i = 0; i < 4; i++)
      cairo_matrix_transform_point (&original_to_transformed,
				    &x[i], &y[i]);

    x_min = floor (x[0]);   y_min = floor (y[0]);
    x_max =  ceil (x[0]);   y_max =  ceil (y[0]);

    for (i = 1; i < 4; i++) {
	if (x[i] < x_min)
	    x_min = floor (x[i]);
	else if (x[i] > x_max)
	    x_max = ceil (x[i]);
	if (y[i] < y_min)
	    y_min = floor (y[i]);
	else if (y[i] > y_max)
	    y_max = ceil (y[i]);
    }

    



    original_to_transformed.x0 -= x_min;
    original_to_transformed.y0 -= y_min;

    
    width  = x_max - x_min;
    height = y_max - y_min;

    transformed_to_original = original_to_transformed;
    status = cairo_matrix_invert (&transformed_to_original);
    if (unlikely (status))
	return status;

    image = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    if (unlikely (image->status))
	return image->status;

    

    _cairo_pattern_init_for_surface (&pattern, &(*surface)->base);
    cairo_pattern_set_matrix (&pattern.base, &transformed_to_original);

    status = _cairo_surface_paint (image,
				   CAIRO_OPERATOR_SOURCE,
				   &pattern.base,
				   NULL);

    _cairo_pattern_fini (&pattern.base);

    if (unlikely (status)) {
	cairo_surface_destroy (image);
	return status;
    }

    


    cairo_matrix_transform_point (&original_to_transformed,
				  &origin_x, &origin_y);

    old_image = (*surface);
    (*surface) = (cairo_image_surface_t *)image;
    cairo_surface_destroy (&old_image->base);

    cairo_surface_set_device_offset (&(*surface)->base,
				     _cairo_lround (origin_x),
				     _cairo_lround (origin_y));
    return CAIRO_STATUS_SUCCESS;
}

static const cairo_unscaled_font_backend_t cairo_ft_unscaled_font_backend = {
    _cairo_ft_unscaled_font_destroy,
#if 0
    _cairo_ft_unscaled_font_create_glyph
#endif
};



typedef struct _cairo_ft_scaled_font {
    cairo_scaled_font_t base;
    cairo_ft_unscaled_font_t *unscaled;
    cairo_ft_options_t ft_options;
} cairo_ft_scaled_font_t;

static const cairo_scaled_font_backend_t _cairo_ft_scaled_font_backend;

#if CAIRO_HAS_FC_FONT



static void
_get_pattern_ft_options (FcPattern *pattern, cairo_ft_options_t *ret)
{
    FcBool antialias, vertical_layout, hinting, autohint, bitmap, embolden;
    cairo_ft_options_t ft_options;
    int rgba;
#ifdef FC_HINT_STYLE
    int hintstyle;
#endif

    _cairo_font_options_init_default (&ft_options.base);
    ft_options.load_flags = FT_LOAD_DEFAULT;
    ft_options.extra_flags = 0;

#ifndef FC_EMBEDDED_BITMAP
#define FC_EMBEDDED_BITMAP "embeddedbitmap"
#endif

    
    if (FcPatternGetBool (pattern,
			  FC_EMBEDDED_BITMAP, 0, &bitmap) != FcResultMatch)
	bitmap = FcFalse;

    
    if (FcPatternGetBool (pattern,
			  FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
	antialias = FcTrue;
    
    if (antialias) {
	cairo_subpixel_order_t subpixel_order;
	int lcd_filter;

	
	if (FcPatternGetBool (pattern,
			      FC_HINTING, 0, &hinting) != FcResultMatch)
	    hinting = FcTrue;

	if (FcPatternGetInteger (pattern,
				 FC_RGBA, 0, &rgba) != FcResultMatch)
	    rgba = FC_RGBA_UNKNOWN;

	switch (rgba) {
	case FC_RGBA_RGB:
	    subpixel_order = CAIRO_SUBPIXEL_ORDER_RGB;
	    break;
	case FC_RGBA_BGR:
	    subpixel_order = CAIRO_SUBPIXEL_ORDER_BGR;
	    break;
	case FC_RGBA_VRGB:
	    subpixel_order = CAIRO_SUBPIXEL_ORDER_VRGB;
	    break;
	case FC_RGBA_VBGR:
	    subpixel_order = CAIRO_SUBPIXEL_ORDER_VBGR;
	    break;
	case FC_RGBA_UNKNOWN:
	case FC_RGBA_NONE:
	default:
	    subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
	    break;
	}

	if (subpixel_order != CAIRO_SUBPIXEL_ORDER_DEFAULT) {
	    ft_options.base.subpixel_order = subpixel_order;
	    ft_options.base.antialias = CAIRO_ANTIALIAS_SUBPIXEL;
	}

	if (FcPatternGetInteger (pattern,
				 FC_LCD_FILTER, 0, &lcd_filter) == FcResultMatch)
	{
	    switch (lcd_filter) {
	    case FC_LCD_NONE:
		ft_options.base.lcd_filter = CAIRO_LCD_FILTER_NONE;
		break;
	    case FC_LCD_DEFAULT:
		ft_options.base.lcd_filter = CAIRO_LCD_FILTER_FIR5;
		break;
	    case FC_LCD_LIGHT:
		ft_options.base.lcd_filter = CAIRO_LCD_FILTER_FIR3;
		break;
	    case FC_LCD_LEGACY:
		ft_options.base.lcd_filter = CAIRO_LCD_FILTER_INTRA_PIXEL;
		break;
	    }
	}

#ifdef FC_HINT_STYLE
	if (FcPatternGetInteger (pattern,
				 FC_HINT_STYLE, 0, &hintstyle) != FcResultMatch)
	    hintstyle = FC_HINT_FULL;

	if (!hinting)
	    hintstyle = FC_HINT_NONE;

	switch (hintstyle) {
	case FC_HINT_NONE:
	    ft_options.base.hint_style = CAIRO_HINT_STYLE_NONE;
	    break;
	case FC_HINT_SLIGHT:
	    ft_options.base.hint_style = CAIRO_HINT_STYLE_SLIGHT;
	    break;
	case FC_HINT_MEDIUM:
	default:
	    ft_options.base.hint_style = CAIRO_HINT_STYLE_MEDIUM;
	    break;
	case FC_HINT_FULL:
	    ft_options.base.hint_style = CAIRO_HINT_STYLE_FULL;
	    break;
	}
#else 
	if (!hinting) {
	    ft_options.base.hint_style = CAIRO_HINT_STYLE_NONE;
	}
#endif 

	
	if (ft_options.base.hint_style == CAIRO_HINT_STYLE_NONE)
	  bitmap = FcFalse;

	if (!bitmap)
	    ft_options.load_flags |= FT_LOAD_NO_BITMAP;

    } else {
	ft_options.base.antialias = CAIRO_ANTIALIAS_NONE;
    }

    
    if (FcPatternGetBool (pattern,
			  FC_AUTOHINT, 0, &autohint) != FcResultMatch)
	autohint = FcFalse;

    if (autohint)
	ft_options.load_flags |= FT_LOAD_FORCE_AUTOHINT;

    if (FcPatternGetBool (pattern,
			  FC_VERTICAL_LAYOUT, 0, &vertical_layout) != FcResultMatch)
	vertical_layout = FcFalse;

    if (vertical_layout)
	ft_options.load_flags |= FT_LOAD_VERTICAL_LAYOUT;

#ifndef FC_EMBOLDEN
#define FC_EMBOLDEN "embolden"
#endif
    if (FcPatternGetBool (pattern,
			  FC_EMBOLDEN, 0, &embolden) != FcResultMatch)
	embolden = FcFalse;

    if (embolden)
	ft_options.extra_flags |= CAIRO_FT_OPTIONS_EMBOLDEN;

    *ret = ft_options;
}
#endif

static void
_cairo_ft_options_merge (cairo_ft_options_t *options,
			 cairo_ft_options_t *other)
{
    int load_flags = other->load_flags;
    int load_target = FT_LOAD_TARGET_NORMAL;

    
    load_flags &= ~(FT_LOAD_TARGET_(FT_LOAD_TARGET_MODE(other->load_flags)));

    if (load_flags & FT_LOAD_NO_HINTING)
	other->base.hint_style = CAIRO_HINT_STYLE_NONE;

    if (other->base.antialias == CAIRO_ANTIALIAS_NONE ||
	options->base.antialias == CAIRO_ANTIALIAS_NONE) {
	options->base.antialias = CAIRO_ANTIALIAS_NONE;
	options->base.subpixel_order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    }

    if (other->base.antialias == CAIRO_ANTIALIAS_SUBPIXEL &&
	(options->base.antialias == CAIRO_ANTIALIAS_DEFAULT ||
	 options->base.antialias == CAIRO_ANTIALIAS_GRAY)) {
	options->base.antialias = CAIRO_ANTIALIAS_SUBPIXEL;
	options->base.subpixel_order = other->base.subpixel_order;
    }

    if (options->base.hint_style == CAIRO_HINT_STYLE_DEFAULT)
	options->base.hint_style = other->base.hint_style;

    if (other->base.hint_style == CAIRO_HINT_STYLE_NONE)
	options->base.hint_style = CAIRO_HINT_STYLE_NONE;

    if (options->base.lcd_filter == CAIRO_LCD_FILTER_DEFAULT)
	options->base.lcd_filter = other->base.lcd_filter;

    if (other->base.lcd_filter == CAIRO_LCD_FILTER_NONE)
	options->base.lcd_filter = CAIRO_LCD_FILTER_NONE;

    if (options->base.antialias == CAIRO_ANTIALIAS_NONE) {
	if (options->base.hint_style == CAIRO_HINT_STYLE_NONE)
	    load_flags |= FT_LOAD_NO_HINTING;
	else
	    load_target = FT_LOAD_TARGET_MONO;
	load_flags |= FT_LOAD_MONOCHROME;
    } else {
	switch (options->base.hint_style) {
	case CAIRO_HINT_STYLE_NONE:
	    load_flags |= FT_LOAD_NO_HINTING;
	    break;
	case CAIRO_HINT_STYLE_SLIGHT:
	    load_target = FT_LOAD_TARGET_LIGHT;
	    break;
	case CAIRO_HINT_STYLE_MEDIUM:
	    break;
	case CAIRO_HINT_STYLE_FULL:
	case CAIRO_HINT_STYLE_DEFAULT:
	    if (options->base.antialias == CAIRO_ANTIALIAS_SUBPIXEL) {
		switch (options->base.subpixel_order) {
		case CAIRO_SUBPIXEL_ORDER_DEFAULT:
		case CAIRO_SUBPIXEL_ORDER_RGB:
		case CAIRO_SUBPIXEL_ORDER_BGR:
		    load_target = FT_LOAD_TARGET_LCD;
		    break;
		case CAIRO_SUBPIXEL_ORDER_VRGB:
		case CAIRO_SUBPIXEL_ORDER_VBGR:
		    load_target = FT_LOAD_TARGET_LCD_V;
		break;
		}
	    }
	    break;
	}
    }

    options->load_flags = load_flags | load_target;
    options->extra_flags = other->extra_flags;
    if (options->base.hint_metrics != CAIRO_HINT_METRICS_OFF)
	options->extra_flags |= CAIRO_FT_OPTIONS_HINT_METRICS;
}

static cairo_status_t
_cairo_ft_font_face_scaled_font_create (void		    *abstract_font_face,
					const cairo_matrix_t	 *font_matrix,
					const cairo_matrix_t	 *ctm,
					const cairo_font_options_t *options,
					cairo_scaled_font_t       **font_out)
{
    cairo_ft_font_face_t *font_face = abstract_font_face;
    cairo_ft_scaled_font_t *scaled_font;
    FT_Face face;
    FT_Size_Metrics *metrics;
    cairo_font_extents_t fs_metrics;
    cairo_status_t status;
    cairo_ft_unscaled_font_t *unscaled;

    assert (font_face->unscaled);

    face = _cairo_ft_unscaled_font_lock_face (font_face->unscaled);
    if (unlikely (face == NULL)) 
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    scaled_font = malloc (sizeof (cairo_ft_scaled_font_t));
    if (unlikely (scaled_font == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL;
    }

    scaled_font->unscaled = unscaled = font_face->unscaled;
    _cairo_unscaled_font_reference (&unscaled->base);

    _cairo_font_options_init_copy (&scaled_font->ft_options.base, options);
    _cairo_ft_options_merge (&scaled_font->ft_options, &font_face->ft_options);

    status = _cairo_scaled_font_init (&scaled_font->base,
			              &font_face->base,
				      font_matrix, ctm, options,
				      &_cairo_ft_scaled_font_backend);
    if (unlikely (status))
	goto CLEANUP_SCALED_FONT;

    status = _cairo_ft_unscaled_font_set_scale (unscaled,
				                &scaled_font->base.scale);
    if (unlikely (status)) {
	

	_cairo_ft_unscaled_font_unlock_face (unscaled);
	_cairo_unscaled_font_destroy (&unscaled->base);
	free (scaled_font);
	return status;
    }


    metrics = &face->size->metrics;

    







    if (scaled_font->base.options.hint_metrics != CAIRO_HINT_METRICS_OFF ||
	face->units_per_EM == 0) {
	double x_factor, y_factor;

	if (unscaled->x_scale == 0)
	    x_factor = 0;
	else
	    x_factor = 1 / unscaled->x_scale;

	if (unscaled->y_scale == 0)
	    y_factor = 0;
	else
	    y_factor = 1 / unscaled->y_scale;

	fs_metrics.ascent =        DOUBLE_FROM_26_6(metrics->ascender) * y_factor;
	fs_metrics.descent =       DOUBLE_FROM_26_6(- metrics->descender) * y_factor;
	fs_metrics.height =        DOUBLE_FROM_26_6(metrics->height) * y_factor;
	if (!_cairo_ft_scaled_font_is_vertical (&scaled_font->base)) {
	    fs_metrics.max_x_advance = DOUBLE_FROM_26_6(metrics->max_advance) * x_factor;
	    fs_metrics.max_y_advance = 0;
	} else {
	    fs_metrics.max_x_advance = 0;
	    fs_metrics.max_y_advance = DOUBLE_FROM_26_6(metrics->max_advance) * y_factor;
	}
    } else {
	double scale = face->units_per_EM;

	fs_metrics.ascent =        face->ascender / scale;
	fs_metrics.descent =       - face->descender / scale;
	fs_metrics.height =        face->height / scale;
	if (!_cairo_ft_scaled_font_is_vertical (&scaled_font->base)) {
	    fs_metrics.max_x_advance = face->max_advance_width / scale;
	    fs_metrics.max_y_advance = 0;
	} else {
	    fs_metrics.max_x_advance = 0;
	    fs_metrics.max_y_advance = face->max_advance_height / scale;
	}
    }

    status = _cairo_scaled_font_set_metrics (&scaled_font->base, &fs_metrics);
    if (unlikely (status))
	goto CLEANUP_SCALED_FONT;

    _cairo_ft_unscaled_font_unlock_face (unscaled);

    *font_out = &scaled_font->base;
    return CAIRO_STATUS_SUCCESS;

  CLEANUP_SCALED_FONT:
    _cairo_unscaled_font_destroy (&unscaled->base);
    free (scaled_font);
  FAIL:
    _cairo_ft_unscaled_font_unlock_face (font_face->unscaled);
    *font_out = _cairo_scaled_font_create_in_error (status);
    return CAIRO_STATUS_SUCCESS; 
}

cairo_bool_t
_cairo_scaled_font_is_ft (cairo_scaled_font_t *scaled_font)
{
    return scaled_font->backend == &_cairo_ft_scaled_font_backend;
}

static void
_cairo_ft_scaled_font_fini (void *abstract_font)
{
    cairo_ft_scaled_font_t *scaled_font = abstract_font;

    if (scaled_font == NULL)
        return;

    _cairo_unscaled_font_destroy (&scaled_font->unscaled->base);
}

static int
_move_to (FT_Vector *to, void *closure)
{
    cairo_path_fixed_t *path = closure;
    cairo_fixed_t x, y;

    x = _cairo_fixed_from_26_6 (to->x);
    y = _cairo_fixed_from_26_6 (to->y);

    if (_cairo_path_fixed_close_path (path) != CAIRO_STATUS_SUCCESS)
	return 1;
    if (_cairo_path_fixed_move_to (path, x, y) != CAIRO_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_line_to (FT_Vector *to, void *closure)
{
    cairo_path_fixed_t *path = closure;
    cairo_fixed_t x, y;

    x = _cairo_fixed_from_26_6 (to->x);
    y = _cairo_fixed_from_26_6 (to->y);

    if (_cairo_path_fixed_line_to (path, x, y) != CAIRO_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_conic_to (FT_Vector *control, FT_Vector *to, void *closure)
{
    cairo_path_fixed_t *path = closure;

    cairo_fixed_t x0, y0;
    cairo_fixed_t x1, y1;
    cairo_fixed_t x2, y2;
    cairo_fixed_t x3, y3;
    cairo_point_t conic;

    if (! _cairo_path_fixed_get_current_point (path, &x0, &y0))
	return 1;

    conic.x = _cairo_fixed_from_26_6 (control->x);
    conic.y = _cairo_fixed_from_26_6 (control->y);

    x3 = _cairo_fixed_from_26_6 (to->x);
    y3 = _cairo_fixed_from_26_6 (to->y);

    x1 = x0 + 2.0/3.0 * (conic.x - x0);
    y1 = y0 + 2.0/3.0 * (conic.y - y0);

    x2 = x3 + 2.0/3.0 * (conic.x - x3);
    y2 = y3 + 2.0/3.0 * (conic.y - y3);

    if (_cairo_path_fixed_curve_to (path,
				    x1, y1,
				    x2, y2,
				    x3, y3) != CAIRO_STATUS_SUCCESS)
	return 1;

    return 0;
}

static int
_cubic_to (FT_Vector *control1, FT_Vector *control2,
	   FT_Vector *to, void *closure)
{
    cairo_path_fixed_t *path = closure;
    cairo_fixed_t x0, y0;
    cairo_fixed_t x1, y1;
    cairo_fixed_t x2, y2;

    x0 = _cairo_fixed_from_26_6 (control1->x);
    y0 = _cairo_fixed_from_26_6 (control1->y);

    x1 = _cairo_fixed_from_26_6 (control2->x);
    y1 = _cairo_fixed_from_26_6 (control2->y);

    x2 = _cairo_fixed_from_26_6 (to->x);
    y2 = _cairo_fixed_from_26_6 (to->y);

    if (_cairo_path_fixed_curve_to (path,
				    x0, y0,
				    x1, y1,
				    x2, y2) != CAIRO_STATUS_SUCCESS)
	return 1;

    return 0;
}

static cairo_status_t
_decompose_glyph_outline (FT_Face		  face,
			  cairo_font_options_t	 *options,
			  cairo_path_fixed_t	**pathp)
{
    static const FT_Outline_Funcs outline_funcs = {
	(FT_Outline_MoveToFunc)_move_to,
	(FT_Outline_LineToFunc)_line_to,
	(FT_Outline_ConicToFunc)_conic_to,
	(FT_Outline_CubicToFunc)_cubic_to,
	0, 
	0, 
    };
    static const FT_Matrix invert_y = {
	DOUBLE_TO_16_16 (1.0), 0,
	0, DOUBLE_TO_16_16 (-1.0),
    };

    FT_GlyphSlot glyph;
    cairo_path_fixed_t *path;
    cairo_status_t status;

    path = _cairo_path_fixed_create ();
    if (!path)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    glyph = face->glyph;

    
    FT_Outline_Transform (&glyph->outline, &invert_y);
    if (FT_Outline_Decompose (&glyph->outline, &outline_funcs, path)) {
	_cairo_path_fixed_destroy (path);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    status = _cairo_path_fixed_close_path (path);
    if (unlikely (status)) {
	_cairo_path_fixed_destroy (path);
	return status;
    }

    *pathp = path;

    return CAIRO_STATUS_SUCCESS;
}




static void
_cairo_ft_scaled_glyph_vertical_layout_bearing_fix (void        *abstract_font,
						    FT_GlyphSlot glyph)
{
    cairo_ft_scaled_font_t *scaled_font = abstract_font;
    FT_Vector vector;

    vector.x = glyph->metrics.vertBearingX - glyph->metrics.horiBearingX;
    vector.y = -glyph->metrics.vertBearingY - glyph->metrics.horiBearingY;

    if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
	FT_Vector_Transform (&vector, &scaled_font->unscaled->Current_Shape);
	FT_Outline_Translate(&glyph->outline, vector.x, vector.y);
    } else if (glyph->format == FT_GLYPH_FORMAT_BITMAP) {
	glyph->bitmap_left += vector.x / 64;
	glyph->bitmap_top  += vector.y / 64;
    }
}

static cairo_int_status_t
_cairo_ft_scaled_glyph_init (void			*abstract_font,
			     cairo_scaled_glyph_t	*scaled_glyph,
			     cairo_scaled_glyph_info_t	 info)
{
    cairo_text_extents_t    fs_metrics;
    cairo_ft_scaled_font_t *scaled_font = abstract_font;
    cairo_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_GlyphSlot glyph;
    FT_Face face;
    FT_Error error;
    int load_flags = scaled_font->ft_options.load_flags;
    FT_Glyph_Metrics *metrics;
    double x_factor, y_factor;
    cairo_bool_t vertical_layout = FALSE;
    cairo_status_t status;

    face = _cairo_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    status = _cairo_ft_unscaled_font_set_scale (scaled_font->unscaled,
				                &scaled_font->base.scale);
    if (unlikely (status))
	goto FAIL;

    
    load_flags |= FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

    if ((info & CAIRO_SCALED_GLYPH_INFO_PATH) != 0 &&
	(info & CAIRO_SCALED_GLYPH_INFO_SURFACE) == 0)
	load_flags |= FT_LOAD_NO_BITMAP;

    



    if (load_flags & FT_LOAD_VERTICAL_LAYOUT) {
	load_flags &= ~FT_LOAD_VERTICAL_LAYOUT;
	vertical_layout = TRUE;
    }

    error = FT_Load_Glyph (scaled_font->unscaled->face,
			   _cairo_scaled_glyph_index(scaled_glyph),
			   load_flags);
    

    if (error == FT_Err_Out_Of_Memory) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FAIL;
    }

    glyph = face->glyph;

#if HAVE_FT_GLYPHSLOT_EMBOLDEN
    


    if (scaled_font->ft_options.extra_flags & CAIRO_FT_OPTIONS_EMBOLDEN)
	FT_GlyphSlot_Embolden (glyph);
#endif

    if (vertical_layout)
	_cairo_ft_scaled_glyph_vertical_layout_bearing_fix (scaled_font, glyph);

    if (info & CAIRO_SCALED_GLYPH_INFO_METRICS) {

	cairo_bool_t hint_metrics = scaled_font->base.options.hint_metrics != CAIRO_HINT_METRICS_OFF;
	


	metrics = &glyph->metrics;

	if (unscaled->x_scale == 0)
	    x_factor = 0;
	else
	    x_factor = 1 / unscaled->x_scale;

	if (unscaled->y_scale == 0)
	    y_factor = 0;
	else
	    y_factor = 1 / unscaled->y_scale;

	









	if (hint_metrics && (load_flags & FT_LOAD_NO_HINTING))
	{
	    FT_Pos x1, x2;
	    FT_Pos y1, y2;
	    FT_Pos advance;

	    if (!vertical_layout) {
		x1 = (metrics->horiBearingX) & -64;
		x2 = (metrics->horiBearingX + metrics->width + 63) & -64;
		y1 = (-metrics->horiBearingY) & -64;
		y2 = (-metrics->horiBearingY + metrics->height + 63) & -64;

		advance = ((metrics->horiAdvance + 32) & -64);

		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (x1) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (y1) * y_factor;

		fs_metrics.width  = DOUBLE_FROM_26_6 (x2 - x1) * x_factor;
		fs_metrics.height  = DOUBLE_FROM_26_6 (y2 - y1) * y_factor;

		fs_metrics.x_advance = DOUBLE_FROM_26_6 (advance) * x_factor;
		fs_metrics.y_advance = 0;
	    } else {
		x1 = (metrics->vertBearingX) & -64;
		x2 = (metrics->vertBearingX + metrics->width + 63) & -64;
		y1 = (metrics->vertBearingY) & -64;
		y2 = (metrics->vertBearingY + metrics->height + 63) & -64;

		advance = ((metrics->vertAdvance + 32) & -64);

		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (x1) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (y1) * y_factor;

		fs_metrics.width  = DOUBLE_FROM_26_6 (x2 - x1) * x_factor;
		fs_metrics.height  = DOUBLE_FROM_26_6 (y2 - y1) * y_factor;

		fs_metrics.x_advance = 0;
		fs_metrics.y_advance = DOUBLE_FROM_26_6 (advance) * y_factor;
	    }
	 } else {
	    fs_metrics.width  = DOUBLE_FROM_26_6 (metrics->width) * x_factor;
	    fs_metrics.height = DOUBLE_FROM_26_6 (metrics->height) * y_factor;

	    if (!vertical_layout) {
		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (metrics->horiBearingX) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (-metrics->horiBearingY) * y_factor;

		if (hint_metrics || glyph->format != FT_GLYPH_FORMAT_OUTLINE)
		    fs_metrics.x_advance = DOUBLE_FROM_26_6 (metrics->horiAdvance) * x_factor;
		else
		    fs_metrics.x_advance = DOUBLE_FROM_16_16 (glyph->linearHoriAdvance) * x_factor;
		fs_metrics.y_advance = 0 * y_factor;
	    } else {
		fs_metrics.x_bearing = DOUBLE_FROM_26_6 (metrics->vertBearingX) * x_factor;
		fs_metrics.y_bearing = DOUBLE_FROM_26_6 (metrics->vertBearingY) * y_factor;

		fs_metrics.x_advance = 0 * x_factor;
		if (hint_metrics || glyph->format != FT_GLYPH_FORMAT_OUTLINE)
		    fs_metrics.y_advance = DOUBLE_FROM_26_6 (metrics->vertAdvance) * y_factor;
		else
		    fs_metrics.y_advance = DOUBLE_FROM_16_16 (glyph->linearVertAdvance) * y_factor;
	    }
	 }

	_cairo_scaled_glyph_set_metrics (scaled_glyph,
					 &scaled_font->base,
					 &fs_metrics);
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_SURFACE) != 0) {
	cairo_image_surface_t	*surface;

	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
	    status = _render_glyph_outline (face, &scaled_font->ft_options.base,
					    &surface);
	} else {
	    status = _render_glyph_bitmap (face, &scaled_font->ft_options.base,
					   &surface);
	    if (likely (status == CAIRO_STATUS_SUCCESS) &&
		unscaled->have_shape)
	    {
		status = _transform_glyph_bitmap (&unscaled->current_shape,
						  &surface);
		if (unlikely (status))
		    cairo_surface_destroy (&surface->base);
	    }
	}
	if (unlikely (status))
	    goto FAIL;

	_cairo_scaled_glyph_set_surface (scaled_glyph,
					 &scaled_font->base,
					 surface);
    }

    if (info & CAIRO_SCALED_GLYPH_INFO_PATH) {
	cairo_path_fixed_t *path = NULL; 

	



	if ((info & CAIRO_SCALED_GLYPH_INFO_SURFACE) != 0) {
	    error = FT_Load_Glyph (face,
				   _cairo_scaled_glyph_index(scaled_glyph),
				   load_flags | FT_LOAD_NO_BITMAP);
	    

	    if (error == FT_Err_Out_Of_Memory) {
		status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
		goto FAIL;
	    }
#if HAVE_FT_GLYPHSLOT_EMBOLDEN
	    


	    if (scaled_font->ft_options.extra_flags & CAIRO_FT_OPTIONS_EMBOLDEN)
		FT_GlyphSlot_Embolden (glyph);
#endif
	    if (vertical_layout)
		_cairo_ft_scaled_glyph_vertical_layout_bearing_fix (scaled_font, glyph);

	}
	if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	    status = _decompose_glyph_outline (face, &scaled_font->ft_options.base,
					       &path);
	else
	    status = CAIRO_INT_STATUS_UNSUPPORTED;

	if (unlikely (status))
	    goto FAIL;

	_cairo_scaled_glyph_set_path (scaled_glyph,
				      &scaled_font->base,
				      path);
    }
 FAIL:
    _cairo_ft_unscaled_font_unlock_face (unscaled);

    return status;
}

static unsigned long
_cairo_ft_ucs4_to_index (void	    *abstract_font,
			 uint32_t    ucs4)
{
    cairo_ft_scaled_font_t *scaled_font = abstract_font;
    cairo_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    FT_UInt index;

    face = _cairo_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return 0;

#if CAIRO_HAS_FC_FONT
    index = FcFreeTypeCharIndex (face, ucs4);
#else
    index = FT_Get_Char_Index (face, ucs4);
#endif

    _cairo_ft_unscaled_font_unlock_face (unscaled);
    return index;
}

static cairo_int_status_t
_cairo_ft_load_truetype_table (void	       *abstract_font,
                              unsigned long     tag,
                              long              offset,
                              unsigned char    *buffer,
                              unsigned long    *length)
{
    cairo_ft_scaled_font_t *scaled_font = abstract_font;
    cairo_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    cairo_status_t status = CAIRO_INT_STATUS_UNSUPPORTED;

    if (_cairo_ft_scaled_font_is_vertical (&scaled_font->base))
        return CAIRO_INT_STATUS_UNSUPPORTED;

#if HAVE_FT_LOAD_SFNT_TABLE
    face = _cairo_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (FT_IS_SFNT (face) &&
	FT_Load_Sfnt_Table (face, tag, offset, buffer, length) == 0)
        status = CAIRO_STATUS_SUCCESS;

    _cairo_ft_unscaled_font_unlock_face (unscaled);
#endif

    return status;
}

static cairo_int_status_t
_cairo_ft_index_to_ucs4(void	        *abstract_font,
			unsigned long    index,
			uint32_t	*ucs4)
{
    cairo_ft_scaled_font_t *scaled_font = abstract_font;
    cairo_ft_unscaled_font_t *unscaled = scaled_font->unscaled;
    FT_Face face;
    FT_ULong  charcode;
    FT_UInt   gindex;

    face = _cairo_ft_unscaled_font_lock_face (unscaled);
    if (!face)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    *ucs4 = (uint32_t) -1;
    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
	if (gindex == index) {
	    *ucs4 = charcode;
	    break;
	}
	charcode = FT_Get_Next_Char (face, charcode, &gindex);
    }

    _cairo_ft_unscaled_font_unlock_face (unscaled);

    return CAIRO_STATUS_SUCCESS;
}

static const cairo_scaled_font_backend_t _cairo_ft_scaled_font_backend = {
    CAIRO_FONT_TYPE_FT,
    _cairo_ft_scaled_font_fini,
    _cairo_ft_scaled_glyph_init,
    NULL,			
    _cairo_ft_ucs4_to_index,
    NULL,			
    _cairo_ft_load_truetype_table,
    _cairo_ft_index_to_ucs4
};



#if CAIRO_HAS_FC_FONT
static cairo_status_t
_cairo_ft_font_face_create_for_pattern (FcPattern *pattern,
					cairo_font_face_t **out);

static cairo_status_t
_cairo_ft_font_face_create_for_toy (cairo_toy_font_face_t   *toy_face,
				    cairo_font_face_t      **font_face)
{
    FcPattern *pattern;
    int fcslant;
    int fcweight;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    pattern = FcPatternCreate ();
    if (!pattern)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    if (!FcPatternAddString (pattern,
		             FC_FAMILY, (unsigned char *) toy_face->family))
    {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    switch (toy_face->slant)
    {
    case CAIRO_FONT_SLANT_ITALIC:
        fcslant = FC_SLANT_ITALIC;
        break;
    case CAIRO_FONT_SLANT_OBLIQUE:
	fcslant = FC_SLANT_OBLIQUE;
        break;
    case CAIRO_FONT_SLANT_NORMAL:
    default:
        fcslant = FC_SLANT_ROMAN;
        break;
    }

    if (!FcPatternAddInteger (pattern, FC_SLANT, fcslant)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    switch (toy_face->weight)
    {
    case CAIRO_FONT_WEIGHT_BOLD:
        fcweight = FC_WEIGHT_BOLD;
        break;
    case CAIRO_FONT_WEIGHT_NORMAL:
    default:
        fcweight = FC_WEIGHT_MEDIUM;
        break;
    }

    if (!FcPatternAddInteger (pattern, FC_WEIGHT, fcweight)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto FREE_PATTERN;
    }

    status = _cairo_ft_font_face_create_for_pattern (pattern, font_face);

 FREE_PATTERN:
    FcPatternDestroy (pattern);

    return status;
}
#endif

static void
_cairo_ft_font_face_destroy (void *abstract_face)
{
    cairo_ft_font_face_t *font_face = abstract_face;

    














    if (font_face->unscaled &&
	font_face->unscaled->from_face &&
	font_face->next == NULL &&
	font_face->unscaled->faces == font_face &&
	CAIRO_REFERENCE_COUNT_GET_VALUE (&font_face->unscaled->base.ref_count) > 1)
    {
	cairo_font_face_reference (&font_face->base);

	_cairo_unscaled_font_destroy (&font_face->unscaled->base);
	font_face->unscaled = NULL;

	return;
    }

    if (font_face->unscaled) {
	cairo_ft_font_face_t *tmp_face = NULL;
	cairo_ft_font_face_t *last_face = NULL;

	
	for (tmp_face = font_face->unscaled->faces;
	     tmp_face;
	     tmp_face = tmp_face->next)
	{
	    if (tmp_face == font_face) {
		if (last_face)
		    last_face->next = tmp_face->next;
		else
		    font_face->unscaled->faces = tmp_face->next;
	    }

	    last_face = tmp_face;
	}

	_cairo_unscaled_font_destroy (&font_face->unscaled->base);
	font_face->unscaled = NULL;
    }

#if CAIRO_HAS_FC_FONT
    if (font_face->pattern) {
	FcPatternDestroy (font_face->pattern);
	cairo_font_face_destroy (font_face->resolved_font_face);
    }
#endif
}

static cairo_font_face_t *
_cairo_ft_font_face_get_implementation (void                     *abstract_face,
					const cairo_matrix_t       *font_matrix,
					const cairo_matrix_t       *ctm,
					const cairo_font_options_t *options)
{
    cairo_ft_font_face_t      *font_face = abstract_face;

    









#if CAIRO_HAS_FC_FONT
    


    if (font_face->pattern) {
	cairo_font_face_t *resolved;

	
	resolved = font_face->resolved_font_face;
	if (resolved != NULL) {
	    if (! FcInitBringUptoDate ()) {
		_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
		return (cairo_font_face_t *) &_cairo_font_face_nil;
	    }

	    if (font_face->resolved_config == FcConfigGetCurrent ())
		return cairo_font_face_reference (resolved);

	    cairo_font_face_destroy (resolved);
	    font_face->resolved_font_face = NULL;
	}

	resolved = _cairo_ft_resolve_pattern (font_face->pattern,
					      font_matrix,
					      ctm,
					      options);
	if (unlikely (resolved->status))
	    return resolved;

	font_face->resolved_font_face = cairo_font_face_reference (resolved);
	font_face->resolved_config = FcConfigGetCurrent ();

	return resolved;
    }
#endif

    return abstract_face;
}

const cairo_font_face_backend_t _cairo_ft_font_face_backend = {
    CAIRO_FONT_TYPE_FT,
#if CAIRO_HAS_FC_FONT
    _cairo_ft_font_face_create_for_toy,
#else
    NULL,
#endif
    _cairo_ft_font_face_destroy,
    _cairo_ft_font_face_scaled_font_create,
    _cairo_ft_font_face_get_implementation
};

#if CAIRO_HAS_FC_FONT
static cairo_status_t
_cairo_ft_font_face_create_for_pattern (FcPattern *pattern,
					cairo_font_face_t **out)
{
    cairo_ft_font_face_t *font_face;

    font_face = malloc (sizeof (cairo_ft_font_face_t));
    if (unlikely (font_face == NULL))
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font_face->unscaled = NULL;
    font_face->next = NULL;

    font_face->pattern = FcPatternDuplicate (pattern);
    if (unlikely (font_face->pattern == NULL)) {
	free (font_face);
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    font_face->resolved_font_face = NULL;
    font_face->resolved_config = NULL;

    _cairo_font_face_init (&font_face->base, &_cairo_ft_font_face_backend);

    *out = &font_face->base;
    return CAIRO_STATUS_SUCCESS;
}
#endif

static cairo_font_face_t *
_cairo_ft_font_face_create (cairo_ft_unscaled_font_t *unscaled,
			    cairo_ft_options_t	     *ft_options)
{
    cairo_ft_font_face_t *font_face, **prev_font_face;

    
    for (font_face = unscaled->faces, prev_font_face = &unscaled->faces;
	 font_face;
	 prev_font_face = &font_face->next, font_face = font_face->next)
    {
	if (font_face->ft_options.load_flags == ft_options->load_flags &&
	    font_face->ft_options.extra_flags == ft_options->extra_flags &&
	    cairo_font_options_equal (&font_face->ft_options.base, &ft_options->base))
	{
	    if (font_face->base.status) {
		
		*prev_font_face = font_face->next;
		break;
	    }

	    if (font_face->unscaled == NULL) {
		


		font_face->unscaled = unscaled;
		_cairo_unscaled_font_reference (&unscaled->base);
		return &font_face->base;
	    } else
		return cairo_font_face_reference (&font_face->base);
	}
    }

    
    font_face = malloc (sizeof (cairo_ft_font_face_t));
    if (unlikely (!font_face)) {
	_cairo_error_throw (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_face_t *)&_cairo_font_face_nil;
    }

    font_face->unscaled = unscaled;
    _cairo_unscaled_font_reference (&unscaled->base);

    font_face->ft_options = *ft_options;

    if (unscaled->faces && unscaled->faces->unscaled == NULL) {
	

	assert (unscaled->from_face && unscaled->faces->next == NULL);
	cairo_font_face_destroy (&unscaled->faces->base);
	unscaled->faces = NULL;
    }

    font_face->next = unscaled->faces;
    unscaled->faces = font_face;

#if CAIRO_HAS_FC_FONT
    font_face->pattern = NULL;
#endif

    _cairo_font_face_init (&font_face->base, &_cairo_ft_font_face_backend);

    return &font_face->base;
}



#if CAIRO_HAS_FC_FONT
static cairo_status_t
_cairo_ft_font_options_substitute (const cairo_font_options_t *options,
				   FcPattern                  *pattern)
{
    FcValue v;

    if (options->antialias != CAIRO_ANTIALIAS_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_ANTIALIAS, 0, &v) == FcResultNoMatch)
	{
	    if (! FcPatternAddBool (pattern,
			            FC_ANTIALIAS,
				    options->antialias != CAIRO_ANTIALIAS_NONE))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	    if (options->antialias != CAIRO_ANTIALIAS_SUBPIXEL) {
		FcPatternDel (pattern, FC_RGBA);
		if (! FcPatternAddInteger (pattern, FC_RGBA, FC_RGBA_NONE))
		    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    }
	}
    }

    if (options->antialias != CAIRO_ANTIALIAS_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_RGBA, 0, &v) == FcResultNoMatch)
	{
	    int rgba;

	    if (options->antialias == CAIRO_ANTIALIAS_SUBPIXEL) {
		switch (options->subpixel_order) {
		case CAIRO_SUBPIXEL_ORDER_DEFAULT:
		case CAIRO_SUBPIXEL_ORDER_RGB:
		default:
		    rgba = FC_RGBA_RGB;
		    break;
		case CAIRO_SUBPIXEL_ORDER_BGR:
		    rgba = FC_RGBA_BGR;
		    break;
		case CAIRO_SUBPIXEL_ORDER_VRGB:
		    rgba = FC_RGBA_VRGB;
		    break;
		case CAIRO_SUBPIXEL_ORDER_VBGR:
		    rgba = FC_RGBA_VBGR;
		    break;
		}
	    } else {
		rgba = FC_RGBA_NONE;
	    }

	    if (! FcPatternAddInteger (pattern, FC_RGBA, rgba))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}
    }

    if (options->lcd_filter != CAIRO_LCD_FILTER_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_LCD_FILTER, 0, &v) == FcResultNoMatch)
	{
	    int lcd_filter;

	    switch (options->lcd_filter) {
	    case CAIRO_LCD_FILTER_NONE:
		lcd_filter = FT_LCD_FILTER_NONE;
		break;
	    case CAIRO_LCD_FILTER_DEFAULT:
	    case CAIRO_LCD_FILTER_INTRA_PIXEL:
		lcd_filter = FT_LCD_FILTER_LEGACY;
		break;
	    case CAIRO_LCD_FILTER_FIR3:
		lcd_filter = FT_LCD_FILTER_LIGHT;
		break;
	    default:
	    case CAIRO_LCD_FILTER_FIR5:
		lcd_filter = FT_LCD_FILTER_DEFAULT;
		break;
	    }

	    if (! FcPatternAddInteger (pattern, FC_LCD_FILTER, lcd_filter))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}
    }

    if (options->hint_style != CAIRO_HINT_STYLE_DEFAULT)
    {
	if (FcPatternGet (pattern, FC_HINTING, 0, &v) == FcResultNoMatch)
	{
	    if (! FcPatternAddBool (pattern,
			            FC_HINTING,
				    options->hint_style != CAIRO_HINT_STYLE_NONE))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}

#ifdef FC_HINT_STYLE
	if (FcPatternGet (pattern, FC_HINT_STYLE, 0, &v) == FcResultNoMatch)
	{
	    int hint_style;

	    switch (options->hint_style) {
	    case CAIRO_HINT_STYLE_NONE:
		hint_style = FC_HINT_NONE;
		break;
	    case CAIRO_HINT_STYLE_SLIGHT:
		hint_style = FC_HINT_SLIGHT;
		break;
	    case CAIRO_HINT_STYLE_MEDIUM:
		hint_style = FC_HINT_MEDIUM;
		break;
	    case CAIRO_HINT_STYLE_FULL:
	    case CAIRO_HINT_STYLE_DEFAULT:
	    default:
		hint_style = FC_HINT_FULL;
		break;
	    }

	    if (! FcPatternAddInteger (pattern, FC_HINT_STYLE, hint_style))
		return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	}
#endif
    }

    return CAIRO_STATUS_SUCCESS;
}












void
cairo_ft_font_options_substitute (const cairo_font_options_t *options,
				  FcPattern                  *pattern)
{
    if (cairo_font_options_status ((cairo_font_options_t *) options))
	return;

    _cairo_ft_font_options_substitute (options, pattern);
}

static cairo_font_face_t *
_cairo_ft_resolve_pattern (FcPattern		      *pattern,
			   const cairo_matrix_t       *font_matrix,
			   const cairo_matrix_t       *ctm,
			   const cairo_font_options_t *font_options)
{
    cairo_status_t status;

    cairo_matrix_t scale;
    FcPattern *resolved;
    cairo_ft_font_transform_t sf;
    FcResult result;
    cairo_ft_unscaled_font_t *unscaled;
    cairo_ft_options_t ft_options;
    cairo_font_face_t *font_face;

    scale = *ctm;
    scale.x0 = scale.y0 = 0;
    cairo_matrix_multiply (&scale,
                           font_matrix,
                           &scale);

    status = _compute_transform (&sf, &scale);
    if (unlikely (status))
	return (cairo_font_face_t *)&_cairo_font_face_nil;

    pattern = FcPatternDuplicate (pattern);
    if (pattern == NULL)
	return (cairo_font_face_t *)&_cairo_font_face_nil;

    if (! FcPatternAddDouble (pattern, FC_PIXEL_SIZE, sf.y_scale)) {
	font_face = (cairo_font_face_t *)&_cairo_font_face_nil;
	goto FREE_PATTERN;
    }

    if (! FcConfigSubstitute (NULL, pattern, FcMatchPattern)) {
	font_face = (cairo_font_face_t *)&_cairo_font_face_nil;
	goto FREE_PATTERN;
    }

    status = _cairo_ft_font_options_substitute (font_options, pattern);
    if (status) {
	font_face = (cairo_font_face_t *)&_cairo_font_face_nil;
	goto FREE_PATTERN;
    }

    FcDefaultSubstitute (pattern);

    resolved = FcFontMatch (NULL, pattern, &result);
    if (!resolved) {
	



	font_face = _cairo_font_face_twin_create_fallback ();
	goto FREE_PATTERN;
    }

    status = _cairo_ft_unscaled_font_create_for_pattern (resolved, &unscaled);
    if (unlikely (status || unscaled == NULL)) {
	font_face = (cairo_font_face_t *)&_cairo_font_face_nil;
	goto FREE_RESOLVED;
    }

    _get_pattern_ft_options (resolved, &ft_options);
    font_face = _cairo_ft_font_face_create (unscaled, &ft_options);
    _cairo_unscaled_font_destroy (&unscaled->base);

FREE_RESOLVED:
    FcPatternDestroy (resolved);

FREE_PATTERN:
    FcPatternDestroy (pattern);

    return font_face;
}






































cairo_font_face_t *
cairo_ft_font_face_create_for_pattern (FcPattern *pattern)
{
    cairo_ft_unscaled_font_t *unscaled;
    cairo_font_face_t *font_face;
    cairo_ft_options_t ft_options;
    cairo_status_t status;

    status = _cairo_ft_unscaled_font_create_for_pattern (pattern, &unscaled);
    if (unlikely (status))
	return (cairo_font_face_t *) &_cairo_font_face_nil;
    if (unlikely (unscaled == NULL)) {
	

	status = _cairo_ft_font_face_create_for_pattern (pattern,
							 &font_face);
	if (unlikely (status))
	    return (cairo_font_face_t *) &_cairo_font_face_nil;

	return font_face;
    }

    _get_pattern_ft_options (pattern, &ft_options);
    font_face = _cairo_ft_font_face_create (unscaled, &ft_options);
    _cairo_unscaled_font_destroy (&unscaled->base);

    return font_face;
}
#endif















































cairo_font_face_t *
cairo_ft_font_face_create_for_ft_face (FT_Face         face,
				       int             load_flags)
{
    cairo_ft_unscaled_font_t *unscaled;
    cairo_font_face_t *font_face;
    cairo_ft_options_t ft_options;
    cairo_status_t status;

    status = _cairo_ft_unscaled_font_create_from_face (face, &unscaled);
    if (unlikely (status))
	return (cairo_font_face_t *)&_cairo_font_face_nil;

    ft_options.load_flags = load_flags;
    ft_options.extra_flags = 0;
    _cairo_font_options_init_default (&ft_options.base);

    font_face = _cairo_ft_font_face_create (unscaled, &ft_options);
    _cairo_unscaled_font_destroy (&unscaled->base);

    return font_face;
}































FT_Face
cairo_ft_scaled_font_lock_face (cairo_scaled_font_t *abstract_font)
{
    cairo_ft_scaled_font_t *scaled_font = (cairo_ft_scaled_font_t *) abstract_font;
    FT_Face face;
    cairo_status_t status;

    if (! _cairo_scaled_font_is_ft (abstract_font)) {
	_cairo_error_throw (CAIRO_STATUS_FONT_TYPE_MISMATCH);
	return NULL;
    }

    if (scaled_font->base.status)
	return NULL;

    face = _cairo_ft_unscaled_font_lock_face (scaled_font->unscaled);
    if (unlikely (face == NULL)) {
	status = _cairo_scaled_font_set_error (&scaled_font->base, CAIRO_STATUS_NO_MEMORY);
	return NULL;
    }

    status = _cairo_ft_unscaled_font_set_scale (scaled_font->unscaled,
				                &scaled_font->base.scale);
    if (unlikely (status)) {
	_cairo_ft_unscaled_font_unlock_face (scaled_font->unscaled);
	status = _cairo_scaled_font_set_error (&scaled_font->base, status);
	return NULL;
    }

    





     CAIRO_MUTEX_UNLOCK (scaled_font->unscaled->mutex);

    return face;
}










void
cairo_ft_scaled_font_unlock_face (cairo_scaled_font_t *abstract_font)
{
    cairo_ft_scaled_font_t *scaled_font = (cairo_ft_scaled_font_t *) abstract_font;

    if (! _cairo_scaled_font_is_ft (abstract_font)) {
	_cairo_error_throw (CAIRO_STATUS_FONT_TYPE_MISMATCH);
	return;
    }

    if (scaled_font->base.status)
	return;

    



    CAIRO_MUTEX_LOCK (scaled_font->unscaled->mutex);

    _cairo_ft_unscaled_font_unlock_face (scaled_font->unscaled);
}





cairo_unscaled_font_t *
_cairo_ft_scaled_font_get_unscaled_font (cairo_scaled_font_t *abstract_font)
{
    cairo_ft_scaled_font_t *scaled_font = (cairo_ft_scaled_font_t *) abstract_font;

    return &scaled_font->unscaled->base;
}

cairo_bool_t
_cairo_ft_scaled_font_is_vertical (cairo_scaled_font_t *scaled_font)
{
    cairo_ft_scaled_font_t *ft_scaled_font;

    if (!_cairo_scaled_font_is_ft (scaled_font))
	return FALSE;

    ft_scaled_font = (cairo_ft_scaled_font_t *) scaled_font;
    if (ft_scaled_font->ft_options.load_flags & FT_LOAD_VERTICAL_LAYOUT)
	return TRUE;
    return FALSE;
}

unsigned int
_cairo_ft_scaled_font_get_load_flags (cairo_scaled_font_t *scaled_font)
{
    cairo_ft_scaled_font_t *ft_scaled_font;

    if (! _cairo_scaled_font_is_ft (scaled_font))
	return 0;

    ft_scaled_font = (cairo_ft_scaled_font_t *) scaled_font;
    return ft_scaled_font->ft_options.load_flags;
}

void
_cairo_ft_font_reset_static_data (void)
{
    _cairo_ft_unscaled_font_map_destroy ();
}
