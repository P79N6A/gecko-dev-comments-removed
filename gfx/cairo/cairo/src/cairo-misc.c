







































#include "cairoint.h"

COMPILE_TIME_ASSERT (CAIRO_STATUS_LAST_STATUS < CAIRO_INT_STATUS_UNSUPPORTED);
COMPILE_TIME_ASSERT (CAIRO_INT_STATUS_LAST_STATUS <= 127);











const char *
cairo_status_to_string (cairo_status_t status)
{
    switch (status) {
    case CAIRO_STATUS_SUCCESS:
	return "success";
    case CAIRO_STATUS_NO_MEMORY:
	return "out of memory";
    case CAIRO_STATUS_INVALID_RESTORE:
	return "cairo_restore without matching cairo_save";
    case CAIRO_STATUS_INVALID_POP_GROUP:
	return "cairo_pop_group without matching cairo_push_group";
    case CAIRO_STATUS_NO_CURRENT_POINT:
	return "no current point defined";
    case CAIRO_STATUS_INVALID_MATRIX:
	return "invalid matrix (not invertible)";
    case CAIRO_STATUS_INVALID_STATUS:
	return "invalid value for an input cairo_status_t";
    case CAIRO_STATUS_NULL_POINTER:
	return "NULL pointer";
    case CAIRO_STATUS_INVALID_STRING:
	return "input string not valid UTF-8";
    case CAIRO_STATUS_INVALID_PATH_DATA:
	return "input path data not valid";
    case CAIRO_STATUS_READ_ERROR:
	return "error while reading from input stream";
    case CAIRO_STATUS_WRITE_ERROR:
	return "error while writing to output stream";
    case CAIRO_STATUS_SURFACE_FINISHED:
	return "the target surface has been finished";
    case CAIRO_STATUS_SURFACE_TYPE_MISMATCH:
	return "the surface type is not appropriate for the operation";
    case CAIRO_STATUS_PATTERN_TYPE_MISMATCH:
	return "the pattern type is not appropriate for the operation";
    case CAIRO_STATUS_INVALID_CONTENT:
	return "invalid value for an input cairo_content_t";
    case CAIRO_STATUS_INVALID_FORMAT:
	return "invalid value for an input cairo_format_t";
    case CAIRO_STATUS_INVALID_VISUAL:
	return "invalid value for an input Visual*";
    case CAIRO_STATUS_FILE_NOT_FOUND:
	return "file not found";
    case CAIRO_STATUS_INVALID_DASH:
	return "invalid value for a dash setting";
    case CAIRO_STATUS_INVALID_DSC_COMMENT:
	return "invalid value for a DSC comment";
    case CAIRO_STATUS_INVALID_INDEX:
	return "invalid index passed to getter";
    case CAIRO_STATUS_CLIP_NOT_REPRESENTABLE:
        return "clip region not representable in desired format";
    case CAIRO_STATUS_TEMP_FILE_ERROR:
	return "error creating or writing to a temporary file";
    case CAIRO_STATUS_INVALID_STRIDE:
	return "invalid value for stride";
    case CAIRO_STATUS_FONT_TYPE_MISMATCH:
	return "the font type is not appropriate for the operation";
    case CAIRO_STATUS_USER_FONT_IMMUTABLE:
	return "the user-font is immutable";
    case CAIRO_STATUS_USER_FONT_ERROR:
	return "error occurred in a user-font callback function";
    case CAIRO_STATUS_NEGATIVE_COUNT:
	return "negative number used where it is not allowed";
    case CAIRO_STATUS_INVALID_CLUSTERS:
	return "input clusters do not represent the accompanying text and glyph arrays";
    case CAIRO_STATUS_INVALID_SLANT:
	return "invalid value for an input #cairo_font_slant_t";
    case CAIRO_STATUS_INVALID_WEIGHT:
	return "input value for an input #cairo_font_weight_t";
    }

    return "<unknown error status>";
}






















cairo_glyph_t *
cairo_glyph_allocate (int num_glyphs)
{
    if (num_glyphs <= 0)
	return NULL;

    return _cairo_malloc_ab (num_glyphs, sizeof (cairo_glyph_t));
}
slim_hidden_def (cairo_glyph_allocate);














void
cairo_glyph_free (cairo_glyph_t *glyphs)
{
    if (glyphs)
	free (glyphs);
}
slim_hidden_def (cairo_glyph_free);





















cairo_text_cluster_t *
cairo_text_cluster_allocate (int num_clusters)
{
    if (num_clusters <= 0)
	return NULL;

    return _cairo_malloc_ab (num_clusters, sizeof (cairo_text_cluster_t));
}
slim_hidden_def (cairo_text_cluster_allocate);














void
cairo_text_cluster_free (cairo_text_cluster_t *clusters)
{
    if (clusters)
	free (clusters);
}
slim_hidden_def (cairo_text_cluster_free);






















cairo_status_t
_cairo_validate_text_clusters (const char		   *utf8,
			       int			    utf8_len,
			       const cairo_glyph_t	   *glyphs,
			       int			    num_glyphs,
			       const cairo_text_cluster_t  *clusters,
			       int			    num_clusters,
			       cairo_text_cluster_flags_t   cluster_flags)
{
    cairo_status_t status;
    unsigned int n_bytes  = 0;
    unsigned int n_glyphs = 0;
    int i;

    for (i = 0; i < num_clusters; i++) {
	int cluster_bytes  = clusters[i].num_bytes;
	int cluster_glyphs = clusters[i].num_glyphs;

	if (cluster_bytes < 0 || cluster_glyphs < 0)
	    goto BAD;

	





	if (cluster_bytes == 0 && cluster_glyphs == 0)
	    goto BAD;

	

	if (n_bytes+cluster_bytes > (unsigned int)utf8_len || n_glyphs+cluster_glyphs > (unsigned int)num_glyphs)
	    goto BAD;

	
	status = _cairo_utf8_to_ucs4 (utf8+n_bytes, cluster_bytes, NULL, NULL);
	if (status)
	    return CAIRO_STATUS_INVALID_CLUSTERS;

	n_bytes  += cluster_bytes ;
	n_glyphs += cluster_glyphs;
    }

    if (n_bytes != (unsigned int) utf8_len || n_glyphs != (unsigned int) num_glyphs) {
      BAD:
	return CAIRO_STATUS_INVALID_CLUSTERS;
    }

    return CAIRO_STATUS_SUCCESS;
}














cairo_bool_t
_cairo_operator_bounded_by_mask (cairo_operator_t op)
{
    switch (op) {
    case CAIRO_OPERATOR_CLEAR:
    case CAIRO_OPERATOR_SOURCE:
    case CAIRO_OPERATOR_OVER:
    case CAIRO_OPERATOR_ATOP:
    case CAIRO_OPERATOR_DEST:
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_DEST_OUT:
    case CAIRO_OPERATOR_XOR:
    case CAIRO_OPERATOR_ADD:
    case CAIRO_OPERATOR_SATURATE:
	return TRUE;
    case CAIRO_OPERATOR_OUT:
    case CAIRO_OPERATOR_IN:
    case CAIRO_OPERATOR_DEST_IN:
    case CAIRO_OPERATOR_DEST_ATOP:
	return FALSE;
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}















cairo_bool_t
_cairo_operator_bounded_by_source (cairo_operator_t op)
{
    switch (op) {
    case CAIRO_OPERATOR_OVER:
    case CAIRO_OPERATOR_ATOP:
    case CAIRO_OPERATOR_DEST:
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_DEST_OUT:
    case CAIRO_OPERATOR_XOR:
    case CAIRO_OPERATOR_ADD:
    case CAIRO_OPERATOR_SATURATE:
	return TRUE;
    case CAIRO_OPERATOR_CLEAR:
    case CAIRO_OPERATOR_SOURCE:
    case CAIRO_OPERATOR_OUT:
    case CAIRO_OPERATOR_IN:
    case CAIRO_OPERATOR_DEST_IN:
    case CAIRO_OPERATOR_DEST_ATOP:
	return FALSE;
    }

    ASSERT_NOT_REACHED;
    return FALSE;
}


void
_cairo_restrict_value (double *value, double min, double max)
{
    if (*value < min)
	*value = min;
    else if (*value > max)
	*value = max;
}




















int
_cairo_lround (double d)
{
    uint32_t top, shift_amount, output;
    union {
        double d;
        uint64_t ui64;
        uint32_t ui32[2];
    } u;

    u.d = d;

    








#if ( defined(FLOAT_WORDS_BIGENDIAN) && !defined(WORDS_BIGENDIAN)) || \
    (!defined(FLOAT_WORDS_BIGENDIAN) &&  defined(WORDS_BIGENDIAN))
    {
        uint32_t temp = u.ui32[0];
        u.ui32[0] = u.ui32[1];
        u.ui32[1] = temp;
    }
#endif

#ifdef WORDS_BIGENDIAN
    #define MSW (0) /* Most Significant Word */
    #define LSW (1) /* Least Significant Word */
#else
    #define MSW (1)
    #define LSW (0)
#endif

    



    top = u.ui32[MSW] >> 20;

    



















    shift_amount = 1053 - (top & 0x7FF);

    


    top >>= 11;

    




    u.ui32[MSW] |= 0x100000;

    





    u.ui64 -= top;

    



    top--;

    







    output = (u.ui32[MSW] << 11) | (u.ui32[LSW] >> 21);

    




















    output >>= shift_amount;

    




    output = (output >> 1) + (output & 1);

    
















    output &= ((shift_amount > 31) - 1);

    

































    output = (output & top) - (output & ~top);

    return output;
#undef MSW
#undef LSW
}


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#if !defined(WINVER) || (WINVER < 0x0500)
# define WINVER 0x0500
#endif
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
# define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <io.h>






FILE *
_cairo_win32_tmpfile (void)
{
#ifdef WINCE 
    return tmpfile();
#else
    DWORD path_len;
    WCHAR path_name[MAX_PATH + 1];
    WCHAR file_name[MAX_PATH + 1];
    HANDLE handle;
    int fd;
    FILE *fp;

    path_len = GetTempPathW (MAX_PATH, path_name);
    if (path_len <= 0 || path_len >= MAX_PATH)
	return NULL;

    if (GetTempFileNameW (path_name, L"ps_", 0, file_name) == 0)
	return NULL;

    handle = CreateFileW (file_name,
			 GENERIC_READ | GENERIC_WRITE,
			 0,
			 NULL,
			 CREATE_ALWAYS,
			 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
			 NULL);
    if (handle == INVALID_HANDLE_VALUE) {
	DeleteFileW (file_name);
	return NULL;
    }

    fd = _open_osfhandle((intptr_t) handle, 0);
    if (fd < 0) {
	CloseHandle (handle);
	return NULL;
    }

    fp = fdopen(fd, "w+b");
    if (fp == NULL) {
	_close(fd);
	return NULL;
    }

    return fp;
#endif 
}

#endif 
