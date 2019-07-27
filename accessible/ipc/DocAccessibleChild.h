





#ifndef mozilla_a11y_DocAccessibleChild_h
#define mozilla_a11y_DocAccessibleChild_h

#include "mozilla/a11y/DocAccessible.h"
#include "mozilla/a11y/PDocAccessibleChild.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace a11y {
class Accessible;
class HyperTextAccessible;

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

  Accessible* IdToAccessible(const uint64_t& aID);
  HyperTextAccessible* IdToHyperTextAccessible(const uint64_t& aID);

  void ShowEvent(AccShowEvent* aShowEvent);

  


  virtual bool RecvState(const uint64_t& aID, uint64_t* aState) MOZ_OVERRIDE;

  


  virtual bool RecvName(const uint64_t& aID, nsString* aName) MOZ_OVERRIDE;

  virtual bool RecvValue(const uint64_t& aID, nsString* aValue) MOZ_OVERRIDE;
  
  


  virtual bool RecvDescription(const uint64_t& aID, nsString* aDesc) MOZ_OVERRIDE;
  virtual bool RecvRelationByType(const uint64_t& aID, const uint32_t& aType,
                                  nsTArray<uint64_t>* aTargets) MOZ_OVERRIDE;
  virtual bool RecvRelations(const uint64_t& aID,
                             nsTArray<RelationTargets>* aRelations)
    MOZ_OVERRIDE;

  virtual bool RecvAttributes(const uint64_t& aID,
                              nsTArray<Attribute> *aAttributes) MOZ_OVERRIDE;

  virtual bool RecvCaretOffset(const uint64_t& aID, int32_t* aOffset)
    MOZ_OVERRIDE;
  virtual bool RecvSetCaretOffset(const uint64_t& aID, const int32_t& aOffset,
                                  bool* aValid) MOZ_OVERRIDE;

  virtual bool RecvCharacterCount(const uint64_t& aID, int32_t* aCount)
     MOZ_OVERRIDE;
  virtual bool RecvSelectionCount(const uint64_t& aID, int32_t* aCount)
     MOZ_OVERRIDE;

  virtual bool RecvTextSubstring(const uint64_t& aID,
                                 const int32_t& aStartOffset,
                                 const int32_t& aEndOffset, nsString* aText)
    MOZ_OVERRIDE;

  virtual bool RecvGetTextAfterOffset(const uint64_t& aID,
                                      const int32_t& aOffset,
                                      const int32_t& aBoundaryType,
                                      nsString* aText, int32_t* aStartOffset,
                                      int32_t* aEndOffset) MOZ_OVERRIDE;
  virtual bool RecvGetTextAtOffset(const uint64_t& aID,
                                   const int32_t& aOffset,
                                   const int32_t& aBoundaryType,
                                   nsString* aText, int32_t* aStartOffset,
                                   int32_t* aEndOffset) MOZ_OVERRIDE;
  virtual bool RecvGetTextBeforeOffset(const uint64_t& aID,
                                       const int32_t& aOffset,
                                       const int32_t& aBoundaryType,
                                       nsString* aText, int32_t* aStartOffset,
                                       int32_t* aEndOffset) MOZ_OVERRIDE;

  virtual bool RecvCharAt(const uint64_t& aID,
                          const int32_t& aOffset,
                          uint16_t* aChar) MOZ_OVERRIDE;

  virtual bool RecvTextAttributes(const uint64_t& aID,
                                  const bool& aIncludeDefAttrs,
                                  const int32_t& aOffset,
                                  nsTArray<Attribute>* aAttributes,
                                  int32_t* aStartOffset,
                                  int32_t* aEndOffset)
    MOZ_OVERRIDE;

  virtual bool RecvDefaultTextAttributes(const uint64_t& aID,
                                         nsTArray<Attribute>* aAttributes)
    MOZ_OVERRIDE;

  virtual bool RecvTextBounds(const uint64_t& aID,
                              const int32_t& aStartOffset,
                              const int32_t& aEndOffset,
                              const uint32_t& aCoordType,
                              nsIntRect* aRetVal) MOZ_OVERRIDE;

  virtual bool RecvCharBounds(const uint64_t& aID,
                              const int32_t& aOffset,
                              const uint32_t& aCoordType,
                              nsIntRect* aRetVal) MOZ_OVERRIDE;

  virtual bool RecvOffsetAtPoint(const uint64_t& aID,
                                 const int32_t& aX,
                                 const int32_t& aY,
                                 const uint32_t& aCoordType,
                                 int32_t* aRetVal) MOZ_OVERRIDE;

  virtual bool RecvSelectionBoundsAt(const uint64_t& aID,
                                     const int32_t& aSelectionNum,
                                     bool* aSucceeded,
                                     nsString* aData,
                                     int32_t* aStartOffset,
                                     int32_t* aEndOffset) MOZ_OVERRIDE;

  virtual bool RecvSetSelectionBoundsAt(const uint64_t& aID,
                                        const int32_t& aSelectionNum,
                                        const int32_t& aStartOffset,
                                        const int32_t& aEndOffset,
                                        bool* aSucceeded) MOZ_OVERRIDE;

  virtual bool RecvAddToSelection(const uint64_t& aID,
                                  const int32_t& aStartOffset,
                                  const int32_t& aEndOffset,
                                  bool* aSucceeded) MOZ_OVERRIDE;

  virtual bool RecvRemoveFromSelection(const uint64_t& aID,
                                       const int32_t& aSelectionNum,
                                       bool* aSucceeded) MOZ_OVERRIDE;

private:
  bool PersistentPropertiesToArray(nsIPersistentProperties* aProps,
                                   nsTArray<Attribute>* aAttributes);

  DocAccessible* mDoc;
};

}
}

#endif
