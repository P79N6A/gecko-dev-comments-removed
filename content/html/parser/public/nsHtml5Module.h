




































#ifndef nsHtml5Module_h__
#define nsHtml5Module_h__

#include "nsIParser.h"

class nsHtml5Module
{
  public:
    static void InitializeStatics();
    static void ReleaseStatics();
    static already_AddRefed<nsIParser> NewHtml5Parser();
    static nsresult Initialize(nsIParser* aParser, nsIDocument* aDoc, nsIURI* aURI, nsISupports* aContainer, nsIChannel* aChannel);
#ifdef DEBUG
  private:
    static PRBool sNsHtml5ModuleInitialized;
#endif
};

#endif 
