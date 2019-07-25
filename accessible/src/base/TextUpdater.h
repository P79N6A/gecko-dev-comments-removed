




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
                PRUint32 aSkipStart);

private:
  TextUpdater();
  TextUpdater(const TextUpdater&);
  TextUpdater& operator=(const TextUpdater&);

  


  void ComputeTextChangeEvents(const nsAString& aStr1,
                               const nsAString& aStr2,
                               PRUint32* aEntries,
                               nsTArray<nsRefPtr<AccEvent> >& aEvents);

  


  inline void FireInsertEvent(const nsAString& aText, PRUint32 aAddlOffset,
                              nsTArray<nsRefPtr<AccEvent> >& aEvents)
  {
    nsRefPtr<AccEvent> event =
      new AccTextChangeEvent(mHyperText, mTextOffset + aAddlOffset,
                             aText, true);
    aEvents.AppendElement(event);
  }

  


  inline void FireDeleteEvent(const nsAString& aText, PRUint32 aAddlOffset,
                              nsTArray<nsRefPtr<AccEvent> >& aEvents)
  {
    nsRefPtr<AccEvent> event =
      new AccTextChangeEvent(mHyperText, mTextOffset + aAddlOffset,
                             aText, false);
    aEvents.AppendElement(event);
  }

  



  const static PRUint32 kMaxStrLen = 1 << 6;

private:
  DocAccessible* mDocument;
  TextLeafAccessible* mTextLeaf;
  HyperTextAccessible* mHyperText;
  PRInt32 mTextOffset;
};

} 
} 

#endif
