


#define ALIGN_TYPE long /* memory alignment */
#define NO_GETENV
#ifdef __cplusplus
#define INLINE inline /* we have them in C++ */
#endif

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT


#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H
#define HAVE_STDLIB_H
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif 

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED
#define GIF_SUPPORTED
#define PPM_SUPPORTED
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED

#undef TWO_FILE_COMMANDLINE	/* You may need this on non-Unix systems */
#undef NEED_SIGNAL_CATCHER	/* Define this if you use jmemname.c */
#undef DONT_USE_B_MODE
	

#endif 
