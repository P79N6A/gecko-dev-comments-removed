







































#ifndef nsBaseFilePicker_h__
#define nsBaseFilePicker_h__

#include "nsISupports.h"
#include "nsIFilePicker.h"
#include "nsISimpleEnumerator.h"
#include "nsArrayEnumerator.h"
#include "nsCOMPtr.h"

class nsIWidget;

#define BASEFILEPICKER_HAS_DISPLAYDIRECTORY 1

class nsBaseFilePicker : public nsIFilePicker
{
public:
  nsBaseFilePicker(); 
  virtual ~nsBaseFilePicker();

  NS_IMETHOD Init(nsIDOMWindow *aParent,
                  const nsAString& aTitle,
                  PRInt16 aMode);

  NS_IMETHOD AppendFilters(PRInt32 filterMask);
  NS_IMETHOD GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHOD SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
#ifdef BASEFILEPICKER_HAS_DISPLAYDIRECTORY 
  NS_IMETHOD GetDisplayDirectory(nsILocalFile * *aDisplayDirectory);
  NS_IMETHOD SetDisplayDirectory(nsILocalFile * aDisplayDirectory);
#endif
  NS_IMETHOD GetAddToRecentDocs(PRBool *aFlag);
  NS_IMETHOD SetAddToRecentDocs(PRBool aFlag);

protected:

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle,
                          PRInt16 aMode) = 0;

  PRBool mAddToRecentDocs;
#ifdef BASEFILEPICKER_HAS_DISPLAYDIRECTORY 
  nsCOMPtr<nsILocalFile> mDisplayDirectory;
#endif
};

#endif 
