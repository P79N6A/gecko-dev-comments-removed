











































#include "xpcprivate.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(nsScriptError, nsIConsoleMessage, nsIScriptError)

nsScriptError::nsScriptError()
    :  mMessage(),
       mSourceName(),
       mLineNumber(0),
       mSourceLine(),
       mColumnNumber(0),
       mFlags(0),
       mCategory()
{
}

nsScriptError::~nsScriptError() {}


NS_IMETHODIMP
nsScriptError::GetMessage(PRUnichar **result) {
    nsresult rv;

    nsCAutoString message;
    rv = ToString(message);
    if (NS_FAILED(rv))
        return rv;

    *result = UTF8ToNewUnicode(message);
    if (!*result)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}


NS_IMETHODIMP
nsScriptError::GetErrorMessage(nsAString& aResult) {
    aResult.Assign(mMessage);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetSourceName(nsAString& aResult) {
    aResult.Assign(mSourceName);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetSourceLine(nsAString& aResult) {
    aResult.Assign(mSourceLine);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetLineNumber(PRUint32 *result) {
    *result = mLineNumber;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetColumnNumber(PRUint32 *result) {
    *result = mColumnNumber;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetFlags(PRUint32 *result) {
    *result = mFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::GetCategory(char **result) {
    *result = ToNewCString(mCategory);
    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::Init(const PRUnichar *message,
                    const PRUnichar *sourceName,
                    const PRUnichar *sourceLine,
                    PRUint32 lineNumber,
                    PRUint32 columnNumber,
                    PRUint32 flags,
                    const char *category)
{
    mMessage.Assign(message);
    mSourceName.Assign(sourceName);
    mLineNumber = lineNumber;
    mSourceLine.Assign(sourceLine);
    mColumnNumber = columnNumber;
    mFlags = flags;
    mCategory.Assign(category);

    return NS_OK;
}

NS_IMETHODIMP
nsScriptError::ToString(nsACString&  aResult)
{
    static const char format0[] =
        "[%s: \"%s\" {file: \"%s\" line: %d column: %d source: \"%s\"}]";
    static const char format1[] =
        "[%s: \"%s\" {file: \"%s\" line: %d}]";
    static const char format2[] =
        "[%s: \"%s\"]";

    static const char error[]   = "JavaScript Error";
    static const char warning[] = "JavaScript Warning";

    const char* severity = !(mFlags & JSREPORT_WARNING) ? error : warning;

    char* temp;
    char* tempMessage = nsnull;
    char* tempSourceName = nsnull;
    char* tempSourceLine = nsnull;

    if(!mMessage.IsEmpty())
        tempMessage = ToNewUTF8String(mMessage);
    if(!mSourceName.IsEmpty())
        tempSourceName = ToNewUTF8String(mSourceName);
    if(!mSourceLine.IsEmpty())
        tempSourceLine = ToNewUTF8String(mSourceLine);

    if(nsnull != tempSourceName && nsnull != tempSourceLine)
        temp = JS_smprintf(format0,
                           severity,
                           tempMessage,
                           tempSourceName,
                           mLineNumber,
                           mColumnNumber,
                           tempSourceLine);
    else if(!mSourceName.IsEmpty())
        temp = JS_smprintf(format1,
                           severity,
                           tempMessage,
                           tempSourceName,
                           mLineNumber);
    else
        temp = JS_smprintf(format2,
                           severity,
                           tempMessage);

    if(nsnull != tempMessage)
        nsMemory::Free(tempMessage);
    if(nsnull != tempSourceName)
        nsMemory::Free(tempSourceName);
    if(nsnull != tempSourceLine)
        nsMemory::Free(tempSourceLine);

    if (!temp)
        return NS_ERROR_OUT_OF_MEMORY;

    aResult.Assign(temp);
    JS_smprintf_free(temp);
    return NS_OK;
}
