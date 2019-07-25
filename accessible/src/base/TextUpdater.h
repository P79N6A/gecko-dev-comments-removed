





































#ifndef TextUpdater_h_
#define TextUpdater_h_

#include "AccEvent.h"
#include "nsHyperTextAccessible.h"





class TextUpdater
{
public:
  


  static void Run(nsDocAccessible* aDocument, nsTextAccessible* aTextLeaf,
                  const nsAString& aNewText);

private:
  TextUpdater(nsDocAccessible* aDocument, nsTextAccessible* aTextLeaf) :
    mDocument(aDocument), mTextLeaf(aTextLeaf), mHyperText(nsnull),
    mTextOffset(-1) { }

  ~TextUpdater()
    { mDocument = nsnull; mTextLeaf = nsnull; mHyperText = nsnull; }

  



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
  nsDocAccessible* mDocument;
  nsTextAccessible* mTextLeaf;
  nsHyperTextAccessible* mHyperText;
  PRInt32 mTextOffset;
};

#endif
