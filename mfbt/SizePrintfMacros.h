







#ifndef mozilla_SizePrintfMacros_h_
#define mozilla_SizePrintfMacros_h_










#if defined(XP_WIN)
#  define PRIoSIZE  "Io"
#  define PRIuSIZE  "Iu"
#  define PRIxSIZE  "Ix"
#  define PRIXSIZE  "IX"
#else
#  define PRIoSIZE  "zo"
#  define PRIuSIZE  "zu"
#  define PRIxSIZE  "zx"
#  define PRIXSIZE  "zX"
#endif

#endif  
