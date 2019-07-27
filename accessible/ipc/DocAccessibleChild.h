





#ifndef mozilla_a11y_DocAccessibleChild_h
#define mozilla_a11y_DocAccessibleChild_h

#include "mozilla/a11y/DocAccessible.h"
#include "mozilla/a11y/PDocAccessibleChild.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace a11y {
class AccShowEvent;

  



class DocAccessibleChild : public PDocAccessibleChild
{
public:
  explicit DocAccessibleChild(DocAccessible* aDoc) :
    mDoc(aDoc)
  { MOZ_COUNT_CTOR(DocAccessibleChild); }
  ~DocAccessibleChild()
  {
    mDoc->SetIPCDoc(nullptr);
    MOZ_COUNT_DTOR(DocAccessibleChild);
  }

  void ShowEvent(AccShowEvent* aShowEvent);

  


  virtual bool RecvState(const uint64_t& aID, uint64_t* aState) MOZ_OVERRIDE;

  


  virtual bool RecvName(const uint64_t& aID, nsString* aName) MOZ_OVERRIDE;

  virtual bool RecvValue(const uint64_t& aID, nsString* aValue) MOZ_OVERRIDE;
  
  


  virtual bool RecvDescription(const uint64_t& aID, nsString* aDesc) MOZ_OVERRIDE;

  virtual bool RecvAttributes(const uint64_t& aID, nsTArray<Attribute> *aAttributes) MOZ_OVERRIDE;
  virtual bool RecvTextSubstring(const uint64_t& aID,
                                 const int32_t& aStartOffset,
                                 const int32_t& aEndOffset, nsString* aText)
    MOZ_OVERRIDE;

private:
  DocAccessible* mDoc;
};

}
}

#endif
