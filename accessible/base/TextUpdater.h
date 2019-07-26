




#ifndef mozilla_a11y_TextUpdater_h__
#define mozilla_a11y_TextUpdater_h__

#include "AccEvent.h"
#include "HyperTextAccessible.h"

namespace mozilla {
namespace a11y {





class TextUpdater
{
public:
  


  static void Run(DocAccessible* aDocument, TextLeafAccessible* aTextLeaf,
                  const nsAString& aNewText);

private:
  TextUpdater(DocAccessible* aDocument, TextLeafAccessible* aTextLeaf) :
    mDocument(aDocument), mTextLeaf(aTextLeaf), mHyperText(nullptr),
    mTextOffset(-1) { }

  ~TextUpdater()
    { mDocument = nullptr; mTextLeaf = nullptr; mHyperText = nullptr; }

  



  void DoUpdate(const nsAString& aNewText, const nsAString& aOldText,
                uint32_t aSkipStart);

private:
  TextUpdater();
  TextUpdater(const TextUpdater&);
  TextUpdater& operator=(const TextUpdater&);

  


  void ComputeTextChangeEvents(const nsAString& aStr1,
                               const nsAString& aStr2,
                               uint32_t* aEntries,
                               nsTArray<nsRefPtr<AccEvent> >& aEvents);

  


  inline void FireInsertEvent(const nsAString& aText, uint32_t aAddlOffset,
                              nsTArray<nsRefPtr<AccEvent> >& aEvents)
  {
    nsRefPtr<AccEvent> event =
      new AccTextChangeEvent(mHyperText, mTextOffset + aAddlOffset,
                             aText, true);
    aEvents.AppendElement(event);
  }

  


  inline void FireDeleteEvent(const nsAString& aText, uint32_t aAddlOffset,
                              nsTArray<nsRefPtr<AccEvent> >& aEvents)
  {
    nsRefPtr<AccEvent> event =
      new AccTextChangeEvent(mHyperText, mTextOffset + aAddlOffset,
                             aText, false);
    aEvents.AppendElement(event);
  }

  



  const static uint32_t kMaxStrLen = 1 << 6;

private:
  DocAccessible* mDocument;
  TextLeafAccessible* mTextLeaf;
  HyperTextAccessible* mHyperText;
  int32_t mTextOffset;
};

} 
} 

#endif
