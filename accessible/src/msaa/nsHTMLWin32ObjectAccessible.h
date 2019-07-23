





































#ifndef _nsHTMLWin32ObjectAccessible_H_
#define _nsHTMLWin32ObjectAccessible_H_

#include "nsIAccessibleWin32Object.h"
#include "nsBaseWidgetAccessible.h"

struct IAccessible;

class nsHTMLWin32ObjectOwnerAccessible : public nsAccessibleWrap
{
public:
  
  
  
  
  
  
  nsHTMLWin32ObjectOwnerAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell, void* aHwnd);
  virtual ~nsHTMLWin32ObjectOwnerAccessible() {}

  
  virtual nsresult Shutdown();

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);

protected:

  
  virtual void CacheChildren();

  void* mHwnd;
  nsRefPtr<nsAccessible> mNativeAccessible;
};










class nsHTMLWin32ObjectAccessible : public nsLeafAccessible,
                                    public nsIAccessibleWin32Object
{
public:

  nsHTMLWin32ObjectAccessible(void* aHwnd);
  virtual ~nsHTMLWin32ObjectAccessible() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLEWIN32OBJECT

protected:
  void* mHwnd;
};

#endif  
