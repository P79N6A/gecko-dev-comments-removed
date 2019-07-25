








#ifndef SkApplication_DEFINED
#define SkApplication_DEFINED

class SkOSWindow;

extern SkOSWindow* create_sk_window(void* hwnd, int argc, char** argv);
extern void application_init();
extern void application_term();

#endif 
