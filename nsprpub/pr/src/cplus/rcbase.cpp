








































#include "rcbase.h"

RCBase::~RCBase() { }

PRSize RCBase::GetErrorTextLength() { return PR_GetErrorTextLength(); }
PRSize RCBase::CopyErrorText(char *text) { return PR_GetErrorText(text); }

void RCBase::SetError(PRErrorCode error, PRInt32 oserror)
    { PR_SetError(error, oserror); }

void RCBase::SetErrorText(PRSize text_length, const char *text)
    { PR_SetErrorText(text_length, text); }


