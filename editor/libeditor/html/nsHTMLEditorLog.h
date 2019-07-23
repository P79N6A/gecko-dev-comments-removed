




































#ifndef nsHTMLEditorLog_h__
#define nsHTMLEditorLog_h__

#include "nsHTMLEditor.h"
#include "nsIEditorLogging.h"
#include "nsIFileStreams.h"
#include "nsCOMPtr.h"

class nsIFile;
class nsEditorTxnLog;




class nsHTMLEditorLog : public nsHTMLEditor,
                        public nsIEditorLogging
{
private:

  nsCOMPtr<nsIOutputStream>     mFileStream;
  nsEditorTxnLog       *mEditorTxnLog;
  PRInt32               mLocked;
  PRInt32               mDepth;

public:

  
  NS_DECL_ISUPPORTS_INHERITED

           nsHTMLEditorLog();
  virtual ~nsHTMLEditorLog();

  
  NS_IMETHOD SetInlineProperty(nsIAtom *aProperty, 
                            const nsAString & aAttribute, 
                            const nsAString & aValue);
  NS_IMETHOD SetParagraphFormat(const nsAString& aParagraphFormat);
  NS_IMETHOD RemoveInlineProperty(nsIAtom *aProperty, const nsAString& aAttribute);
  NS_IMETHOD DeleteSelection(nsIEditor::EDirection aAction);
  NS_IMETHOD InsertText(const nsAString& aStringToInsert);
  NS_IMETHOD InsertLineBreak();
  NS_IMETHOD Undo(PRUint32 aCount);
  NS_IMETHOD Redo(PRUint32 aCount);
  NS_IMETHOD BeginTransaction();
  NS_IMETHOD EndTransaction();
  NS_IMETHOD SelectAll();
  NS_IMETHOD BeginningOfDocument();
  NS_IMETHOD EndOfDocument();

  NS_IMETHOD Cut();
  NS_IMETHOD Copy();
  NS_IMETHOD Paste(PRInt32 aSelectionType);
  NS_IMETHOD PasteAsQuotation(PRInt32 aSelectionType);
  NS_IMETHOD PasteAsPlaintextQuotation(PRInt32 aSelectionType);
  NS_IMETHOD PasteAsCitedQuotation(const nsAString& aCitation,
                                   PRInt32 aSelectionType);
  NS_IMETHOD InsertAsQuotation(const nsAString& aQuotedText, nsIDOMNode** aNodeInserted);
  NS_IMETHOD InsertAsPlaintextQuotation(const nsAString& aQuotedText,
                                        PRBool aAddCites,
                                        nsIDOMNode** aNodeInserted);
  NS_IMETHOD InsertAsCitedQuotation(const nsAString& aQuotedText, const nsAString& aCitation, 
                                    PRBool aInsertHTML,
                                    nsIDOMNode** aNodeInserted);

  NS_IMETHOD SetDocumentTitle(const nsAString& aTitle);

  NS_IMETHOD SetBackgroundColor(const nsAString& aColor);
  NS_IMETHOD SetBodyAttribute(const nsAString& aAttr, const nsAString& aValue);
  NS_IMETHOD MakeOrChangeList(const nsAString& aListType, PRBool entireList, const nsAString& aBulletType);
  NS_IMETHOD Indent(const nsAString& aIndent);
  NS_IMETHOD Align(const nsAString& aAlign);
  NS_IMETHOD InsertElementAtSelection(nsIDOMElement* aElement, PRBool aDeleteSelection);
  NS_IMETHOD InsertLinkAroundSelection(nsIDOMElement* aAnchorElement);
  
  
  NS_IMETHOD InsertTableCell(PRInt32 aNumber, PRBool aAfter);
  NS_IMETHOD InsertTableColumn(PRInt32 aNumber, PRBool aAfter);
  NS_IMETHOD InsertTableRow(PRInt32 aNumber, PRBool aAfter);
  NS_IMETHOD DeleteTable();
  NS_IMETHOD DeleteTableCell(PRInt32 aNumber);
  NS_IMETHOD DeleteTableCellContents();
  NS_IMETHOD DeleteTableColumn(PRInt32 aNumber);
  NS_IMETHOD DeleteTableRow(PRInt32 aNumber);
  NS_IMETHOD JoinTableCells(PRBool aMergeNonContiguousContents);
  NS_IMETHOD SplitTableCell();
  NS_IMETHOD NormalizeTable(nsIDOMElement *aTable);
  NS_IMETHOD SwitchTableCellHeaderType(nsIDOMElement *aSourceCell, nsIDOMElement **aNewCell);

  NS_IMETHOD StartLogging(nsIFile *aLogFile);
  NS_IMETHOD StopLogging();

  
  nsresult Write(const char *aBuffer);
  nsresult WriteInt(PRInt32 aInt);
  nsresult Flush();
  nsresult PrintUnicode(const nsAString &aString);
  nsresult PrintSelection();
  nsresult PrintNode(nsIDOMNode *aNode, PRInt32 aDepth=0);
  nsresult PrintElementNode(nsIDOMNode *aNode, PRInt32 aDepth);
  nsresult PrintTextNode(nsIDOMNode *aNode, PRInt32 aDepth);
  nsresult PrintAttributeNode(nsIDOMNode *aNode, PRInt32 aDepth=0);
  nsresult PrintNodeChildren(nsIDOMNode *aNode, PRInt32 aDepth=0);
  nsresult GetNodeTreeOffsets(nsIDOMNode *aNode, PRInt32 **aResult, PRInt32 *aLength);
  nsresult Lock();
  nsresult Unlock();
};

class nsAutoHTMLEditorLogLock
{
  nsHTMLEditorLog *mLog;

public:

  nsAutoHTMLEditorLogLock(nsHTMLEditorLog *aLog)
  {
    mLog = aLog;

    if (mLog)
      mLog->Lock();
  }

  ~nsAutoHTMLEditorLogLock()
  {
    if (mLog)
      mLog->Unlock();
  }
};

#endif 
