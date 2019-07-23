








































#if defined(_RCRUNTIME_H)
#else
#define _RCRUNTIME_H

#include <prerror.h>











class PR_IMPLEMENT(RCBase)
{
public:
    virtual ~RCBase();

    static void AbortSelf();

    static PRErrorCode GetError();
    static PRInt32 GetOSError();

    static PRSize GetErrorTextLength();
    static PRSize CopyErrorText(char *text);

    static void SetError(PRErrorCode error, PRInt32 oserror);
    static void SetErrorText(PRSize textLength, const char *text);

protected:
    RCBase() { }
};  

inline PRErrorCode RCBase::GetError() { return PR_GetError(); }
inline PRInt32 RCBase::GetOSError() { return PR_GetOSError(); }

#endif  


