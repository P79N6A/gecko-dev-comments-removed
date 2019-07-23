










































#define HAVE_PROTOTYPES 
#define HAVE_UNSIGNED_CHAR 
#define HAVE_UNSIGNED_SHORT 


#ifndef HAVE_STDDEF_H 
#define HAVE_STDDEF_H 
#endif 
#ifndef HAVE_STDLIB_H
#define HAVE_STDLIB_H 
#endif 
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES

#undef INCOMPLETE_TYPES_BROKEN




#undef CHAR_IS_UNSIGNED




#ifdef JPEG_INTERNALS




#undef RIGHT_SHIFT_IS_UNSIGNED

#ifdef XP_MAC                   

#define ALIGN_TYPE long         /* for sane memory alignment */
#define NO_GETENV

#endif 

#endif 





#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED
#define GIF_SUPPORTED
#define PPM_SUPPORTED
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED

#undef TWO_FILE_COMMANDLINE
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT

#endif 
