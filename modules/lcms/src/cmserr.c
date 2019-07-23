






















#include "lcms.h"





void cdecl cmsSignalError(int ErrorCode, const char *ErrorText, ...);

int  LCMSEXPORT cmsErrorAction(int lAbort);
void LCMSEXPORT cmsSetErrorHandler(cmsErrorHandlerFunction Fn);




static int nDoAbort = LCMS_ERROR_ABORT;
static cmsErrorHandlerFunction UserErrorHandler = (cmsErrorHandlerFunction) NULL;


int LCMSEXPORT cmsErrorAction(int nAction)
{
       int nOld = nDoAbort;    
       nDoAbort = nAction;

       return nOld;
}

void LCMSEXPORT cmsSetErrorHandler(cmsErrorHandlerFunction Fn)
{
       UserErrorHandler = Fn;
}





void cmsSignalError(int ErrorCode, const char *ErrorText, ...)
{
       va_list args;
       
       if (nDoAbort == LCMS_ERROR_IGNORE) return;

        va_start(args, ErrorText);

        if (UserErrorHandler != NULL) {

            char Buffer[1024];

            vsnprintf(Buffer, 1023, ErrorText, args);
            va_end(args);   

            if (UserErrorHandler(ErrorCode, Buffer)) {     

                return;
                }
         }

#if defined( __CONSOLE__ ) || defined( NON_WINDOWS )

              fprintf(stderr, "lcms: Error #%d; ", ErrorCode);
              vfprintf(stderr, ErrorText, args);
              fprintf(stderr, "\n");
              va_end(args);

              if (nDoAbort == LCMS_ERROR_ABORT) exit(1);
#else
              {
#ifdef WINCE
              UINT type = MB_OK|MB_ICONSTOP|MB_APPLMODAL;
#else
              UINT type = MB_OK|MB_ICONSTOP|MB_TASKMODAL;
#endif

              char Buffer1[1024];
              char Buffer2[256];

              snprintf(Buffer1,  767, "Error #%x; ", ErrorCode);
              vsnprintf(Buffer2, 255, ErrorText, args);
              strcat(Buffer1, Buffer2);
              MessageBox(NULL, Buffer1, "Little cms", type);
              va_end(args);

              if (nDoAbort == LCMS_ERROR_ABORT) {

#ifdef __BORLANDC__
                    _cexit();
#endif

#ifndef WINCE
                  FatalAppExitW(0, L"lcms is terminating application");
#endif
              }
              }
#endif
}
