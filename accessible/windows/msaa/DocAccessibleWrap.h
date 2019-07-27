





#ifndef mozilla_a11y_DocAccessibleWrap_h__
#define mozilla_a11y_DocAccessibleWrap_h__

#include "DocAccessible.h"

namespace mozilla {
namespace a11y {

class DocAccessibleWrap : public DocAccessible
{
public:
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell);
  virtual ~DocAccessibleWrap();

  DECL_IUNKNOWN_INHERITED

  

    
    virtual  HRESULT STDMETHODCALLTYPE get_accValue( 
         VARIANT varChild,
         BSTR __RPC_FAR *pszValue);

  
  virtual void Shutdown();

  
  virtual void* GetNativeWindow() const;

  


#ifdef _WIN64
  void AddID(uint32_t aID, AccessibleWrap* aAcc)
    { mIDToAccessibleMap.Put(aID, aAcc); }
  void RemoveID(uint32_t aID) { mIDToAccessibleMap.Remove(aID); }
  AccessibleWrap* GetAccessibleByID(uint32_t aID) const
    { return mIDToAccessibleMap.Get(aID); }
#endif

protected:
  
  virtual void DoInitialUpdate();

protected:
  void* mHWND;

  


#ifdef _WIN64
  nsDataHashtable<nsUint32HashKey, AccessibleWrap*> mIDToAccessibleMap;
#endif
};

} 
} 

#endif
