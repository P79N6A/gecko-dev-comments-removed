





































#ifndef nsMorkHistoryImporter_h_
#define nsMorkHistoryImporter_h_

#include "nsINavHistoryService.h"
#include "nsMorkReader.h"

template<class E> class nsTArray;






class nsMorkHistoryImporter : public nsIMorkHistoryImporter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMORKHISTORYIMPORTER

private:
  
  static PLDHashOperator PR_CALLBACK
  AddToHistoryCB(const nsCSubstring &aRowID,
                 const nsTArray<nsCString> *aValues,
                 void *aData);
};

#endif 
