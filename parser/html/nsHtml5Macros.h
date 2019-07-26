





















#ifndef nsHtml5Macros_h
#define nsHtml5Macros_h

#define NS_HTML5_CONTINUE(target) \
  goto target

#define NS_HTML5_BREAK(target) \
  goto target ## _end

#endif 
