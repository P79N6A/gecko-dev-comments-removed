





































#include "EmbedGlobalHistory.h"

#include <qstring.h>


NS_IMPL_ISUPPORTS1(EmbedGlobalHistory, nsIGlobalHistory)

EmbedGlobalHistory::EmbedGlobalHistory()
{
  
}

EmbedGlobalHistory::~EmbedGlobalHistory()
{
  
}


NS_IMETHODIMP EmbedGlobalHistory::AddPage(const char *aURL)
{
    qDebug("here");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP EmbedGlobalHistory::IsVisited(const char *aURL, PRBool *_retval)
{
    qDebug("HERE");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP EmbedGlobalHistory::Init()
{
    qDebug("initing embedglobal");
    return NS_OK;
}
