





#ifndef nsBaseFilePicker_h__
#define nsBaseFilePicker_h__

#include "nsISupports.h"
#include "nsIFilePicker.h"
#include "nsISimpleEnumerator.h"
#include "nsArrayEnumerator.h"
#include "nsCOMPtr.h"

class nsPIDOMWindow;
class nsIWidget;

class nsBaseFilePicker : public nsIFilePicker
{
public:
  nsBaseFilePicker(); 
  virtual ~nsBaseFilePicker();

  NS_IMETHOD Init(nsIDOMWindow *aParent,
                  const nsAString& aTitle,
                  int16_t aMode);

  NS_IMETHOD Open(nsIFilePickerShownCallback *aCallback);
  NS_IMETHOD AppendFilters(int32_t filterMask);
  NS_IMETHOD GetFilterIndex(int32_t *aFilterIndex);
  NS_IMETHOD SetFilterIndex(int32_t aFilterIndex);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD GetDisplayDirectory(nsIFile * *aDisplayDirectory);
  NS_IMETHOD SetDisplayDirectory(nsIFile * aDisplayDirectory);
  NS_IMETHOD GetAddToRecentDocs(bool *aFlag);
  NS_IMETHOD SetAddToRecentDocs(bool aFlag);
  NS_IMETHOD GetMode(int16_t *aMode);

  NS_IMETHOD GetDomfile(nsISupports** aDomfile);
  NS_IMETHOD GetDomfiles(nsISimpleEnumerator** aDomfiles);

protected:

  virtual void InitNative(nsIWidget *aParent, const nsAString& aTitle) = 0;

  bool mAddToRecentDocs;
  nsCOMPtr<nsIFile> mDisplayDirectory;

  
  nsCOMPtr<nsPIDOMWindow> mParent;
  int16_t mMode;
};

#endif 
