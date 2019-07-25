




































#ifndef nsHTMLEditUtils_h__
#define nsHTMLEditUtils_h__

#include "prtypes.h"  
#include "nsError.h"  
class nsIEditor;
class nsIDOMNode;

class nsHTMLEditUtils
{
public:
  
  static PRBool IsBig(nsIDOMNode *aNode);
  static PRBool IsSmall(nsIDOMNode *aNode);

  
  static PRBool IsInlineStyle(nsIDOMNode *aNode);
  static PRBool IsFormatNode(nsIDOMNode *aNode);
  static PRBool IsNodeThatCanOutdent(nsIDOMNode *aNode);
  static PRBool IsHeader(nsIDOMNode *aNode);
  static PRBool IsParagraph(nsIDOMNode *aNode);
  static PRBool IsHR(nsIDOMNode *aNode);
  static PRBool IsListItem(nsIDOMNode *aNode);
  static PRBool IsTable(nsIDOMNode *aNode);
  static PRBool IsTableRow(nsIDOMNode *aNode);
  static PRBool IsTableElement(nsIDOMNode *aNode);
  static PRBool IsTableElementButNotTable(nsIDOMNode *aNode);
  static PRBool IsTableCell(nsIDOMNode *aNode);
  static PRBool IsTableCellOrCaption(nsIDOMNode *aNode);
  static PRBool IsList(nsIDOMNode *aNode);
  static PRBool IsOrderedList(nsIDOMNode *aNode);
  static PRBool IsUnorderedList(nsIDOMNode *aNode);
  static PRBool IsBlockquote(nsIDOMNode *aNode);
  static PRBool IsPre(nsIDOMNode *aNode);
  static PRBool IsAddress(nsIDOMNode *aNode);
  static PRBool IsAnchor(nsIDOMNode *aNode);
  static PRBool IsImage(nsIDOMNode *aNode);
  static PRBool IsLink(nsIDOMNode *aNode);
  static PRBool IsNamedAnchor(nsIDOMNode *aNode);
  static PRBool IsDiv(nsIDOMNode *aNode);
  static PRBool IsMozDiv(nsIDOMNode *aNode);
  static PRBool IsMailCite(nsIDOMNode *aNode);
  static PRBool IsFormWidget(nsIDOMNode *aNode);
  static PRBool SupportsAlignAttr(nsIDOMNode *aNode);
  static PRBool CanContain(PRInt32 aParent, PRInt32 aChild);
  static PRBool IsContainer(PRInt32 aTag);
};

#endif 

