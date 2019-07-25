




































#ifndef nsHTMLEditUtils_h__
#define nsHTMLEditUtils_h__

#include "prtypes.h"  
#include "nsError.h"  
class nsIEditor;
class nsIDOMNode;

class nsHTMLEditUtils
{
public:
  
  static bool IsBig(nsIDOMNode *aNode);
  static bool IsSmall(nsIDOMNode *aNode);

  
  static bool IsInlineStyle(nsIDOMNode *aNode);
  static bool IsFormatNode(nsIDOMNode *aNode);
  static bool IsNodeThatCanOutdent(nsIDOMNode *aNode);
  static bool IsHeader(nsIDOMNode *aNode);
  static bool IsParagraph(nsIDOMNode *aNode);
  static bool IsHR(nsIDOMNode *aNode);
  static bool IsListItem(nsIDOMNode *aNode);
  static bool IsTable(nsIDOMNode *aNode);
  static bool IsTableRow(nsIDOMNode *aNode);
  static bool IsTableElement(nsIDOMNode *aNode);
  static bool IsTableElementButNotTable(nsIDOMNode *aNode);
  static bool IsTableCell(nsIDOMNode *aNode);
  static bool IsTableCellOrCaption(nsIDOMNode *aNode);
  static bool IsList(nsIDOMNode *aNode);
  static bool IsOrderedList(nsIDOMNode *aNode);
  static bool IsUnorderedList(nsIDOMNode *aNode);
  static bool IsBlockquote(nsIDOMNode *aNode);
  static bool IsPre(nsIDOMNode *aNode);
  static bool IsAddress(nsIDOMNode *aNode);
  static bool IsAnchor(nsIDOMNode *aNode);
  static bool IsImage(nsIDOMNode *aNode);
  static bool IsLink(nsIDOMNode *aNode);
  static bool IsNamedAnchor(nsIDOMNode *aNode);
  static bool IsDiv(nsIDOMNode *aNode);
  static bool IsMozDiv(nsIDOMNode *aNode);
  static bool IsMailCite(nsIDOMNode *aNode);
  static bool IsFormWidget(nsIDOMNode *aNode);
  static bool SupportsAlignAttr(nsIDOMNode *aNode);
  static bool CanContain(PRInt32 aParent, PRInt32 aChild);
  static bool IsContainer(PRInt32 aTag);
};

#endif 

