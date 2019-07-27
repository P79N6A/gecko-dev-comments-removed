




#include "mozilla/ArrayUtils.h"         
#include "mozilla/Assertions.h"         
#include "mozilla/dom/Element.h"        
#include "nsAString.h"                  
#include "nsCOMPtr.h"                   
#include "nsCaseTreatment.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsGkAtoms.h"                  
#include "nsHTMLEditUtils.h"
#include "nsHTMLTags.h"
#include "nsIAtom.h"                    
#include "nsIDOMHTMLAnchorElement.h"    
#include "nsIDOMNode.h"                 
#include "nsNameSpaceManager.h"        
#include "nsLiteralString.h"            
#include "nsString.h"                   
#include "nsTextEditUtils.h"            

using namespace mozilla;



bool 
nsHTMLEditUtils::IsBig(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::big);
}





bool 
nsHTMLEditUtils::IsInlineStyle(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsInlineStyle");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsInlineStyle(node);
}

bool
nsHTMLEditUtils::IsInlineStyle(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  nsIAtom* nodeAtom = aNode->Tag();
  return (nodeAtom == nsGkAtoms::b)
      || (nodeAtom == nsGkAtoms::i)
      || (nodeAtom == nsGkAtoms::u)
      || (nodeAtom == nsGkAtoms::tt)
      || (nodeAtom == nsGkAtoms::s)
      || (nodeAtom == nsGkAtoms::strike)
      || (nodeAtom == nsGkAtoms::big)
      || (nodeAtom == nsGkAtoms::small)
      || (nodeAtom == nsGkAtoms::sub)
      || (nodeAtom == nsGkAtoms::sup)
      || (nodeAtom == nsGkAtoms::font);
}




bool
nsHTMLEditUtils::IsFormatNode(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsFormatNode");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsFormatNode(node);
}

bool
nsHTMLEditUtils::IsFormatNode(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  nsIAtom* nodeAtom = aNode->Tag();
  return (nodeAtom == nsGkAtoms::p)
      || (nodeAtom == nsGkAtoms::pre)
      || (nodeAtom == nsGkAtoms::h1)
      || (nodeAtom == nsGkAtoms::h2)
      || (nodeAtom == nsGkAtoms::h3)
      || (nodeAtom == nsGkAtoms::h4)
      || (nodeAtom == nsGkAtoms::h5)
      || (nodeAtom == nsGkAtoms::h6)
      || (nodeAtom == nsGkAtoms::address);
}




bool
nsHTMLEditUtils::IsNodeThatCanOutdent(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsNodeThatCanOutdent");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aNode);
  return (nodeAtom == nsGkAtoms::ul)
      || (nodeAtom == nsGkAtoms::ol)
      || (nodeAtom == nsGkAtoms::dl)
      || (nodeAtom == nsGkAtoms::li)
      || (nodeAtom == nsGkAtoms::dd)
      || (nodeAtom == nsGkAtoms::dt)
      || (nodeAtom == nsGkAtoms::blockquote);
}



bool 
nsHTMLEditUtils::IsSmall(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::small);
}





 



bool 
nsHTMLEditUtils::IsHeader(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsHeader");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aNode);
  return (nodeAtom == nsGkAtoms::h1)
      || (nodeAtom == nsGkAtoms::h2)
      || (nodeAtom == nsGkAtoms::h3)
      || (nodeAtom == nsGkAtoms::h4)
      || (nodeAtom == nsGkAtoms::h5)
      || (nodeAtom == nsGkAtoms::h6);
}





bool 
nsHTMLEditUtils::IsParagraph(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::p);
}





bool 
nsHTMLEditUtils::IsHR(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::hr);
}





bool 
nsHTMLEditUtils::IsListItem(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsListItem");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsListItem(node);
}

bool
nsHTMLEditUtils::IsListItem(nsINode* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsGkAtoms::li)
      || (nodeAtom == nsGkAtoms::dd)
      || (nodeAtom == nsGkAtoms::dt);
}





bool
nsHTMLEditUtils::IsTableElement(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditor::IsTableElement");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsTableElement(node);
}

bool
nsHTMLEditUtils::IsTableElement(nsINode* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsGkAtoms::table)
      || (nodeAtom == nsGkAtoms::tr)
      || (nodeAtom == nsGkAtoms::td)
      || (nodeAtom == nsGkAtoms::th)
      || (nodeAtom == nsGkAtoms::thead)
      || (nodeAtom == nsGkAtoms::tfoot)
      || (nodeAtom == nsGkAtoms::tbody)
      || (nodeAtom == nsGkAtoms::caption);
}




bool 
nsHTMLEditUtils::IsTableElementButNotTable(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditor::IsTableElementButNotTable");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsTableElementButNotTable(node);
}

bool
nsHTMLEditUtils::IsTableElementButNotTable(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  nsCOMPtr<nsIAtom> nodeAtom = aNode->Tag();
  return (nodeAtom == nsGkAtoms::tr)
      || (nodeAtom == nsGkAtoms::td)
      || (nodeAtom == nsGkAtoms::th)
      || (nodeAtom == nsGkAtoms::thead)
      || (nodeAtom == nsGkAtoms::tfoot)
      || (nodeAtom == nsGkAtoms::tbody)
      || (nodeAtom == nsGkAtoms::caption);
}




bool
nsHTMLEditUtils::IsTable(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::table);
}

bool
nsHTMLEditUtils::IsTable(nsINode* aNode)
{
  return aNode && aNode->IsElement() && aNode->Tag() == nsGkAtoms::table;
}




bool 
nsHTMLEditUtils::IsTableRow(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::tr);
}





bool 
nsHTMLEditUtils::IsTableCell(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsTableCell");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsTableCell(node);
}

bool
nsHTMLEditUtils::IsTableCell(nsINode* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsGkAtoms::td)
      || (nodeAtom == nsGkAtoms::th);
}





bool 
nsHTMLEditUtils::IsTableCellOrCaption(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsTableCell");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aNode);
  return (nodeAtom == nsGkAtoms::td)
      || (nodeAtom == nsGkAtoms::th)
      || (nodeAtom == nsGkAtoms::caption);
}





bool
nsHTMLEditUtils::IsList(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsList");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsList(node);
}

bool
nsHTMLEditUtils::IsList(nsINode* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsGkAtoms::ul)
      || (nodeAtom == nsGkAtoms::ol)
      || (nodeAtom == nsGkAtoms::dl);
}





bool 
nsHTMLEditUtils::IsOrderedList(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::ol);
}





bool 
nsHTMLEditUtils::IsUnorderedList(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::ul);
}





bool 
nsHTMLEditUtils::IsBlockquote(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::blockquote);
}





bool 
nsHTMLEditUtils::IsPre(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::pre);
}





bool 
nsHTMLEditUtils::IsImage(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::img);
}

bool 
nsHTMLEditUtils::IsLink(nsIDOMNode *aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsLink(node);
}

bool
nsHTMLEditUtils::IsLink(nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aNode);
  if (anchor)
  {
    nsAutoString tmpText;
    if (NS_SUCCEEDED(anchor->GetHref(tmpText)) && !tmpText.IsEmpty()) {
      return true;
    }
  }
  return false;
}

bool 
nsHTMLEditUtils::IsNamedAnchor(nsIDOMNode *aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsNamedAnchor(node);
}

bool
nsHTMLEditUtils::IsNamedAnchor(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  if (!aNode->IsElement() || !aNode->AsElement()->IsHTML(nsGkAtoms::a)) {
    return false;
  }

  nsAutoString text;
  return aNode->AsElement()->GetAttr(kNameSpaceID_None, nsGkAtoms::name,
                                     text) && !text.IsEmpty();
}





bool 
nsHTMLEditUtils::IsDiv(nsIDOMNode* aNode)
{
  return nsEditor::NodeIsType(aNode, nsGkAtoms::div);
}





bool 
nsHTMLEditUtils::IsMozDiv(nsIDOMNode* aNode)
{
  if (IsDiv(aNode) && nsTextEditUtils::HasMozAttr(aNode)) return true;
  return false;
}

bool
nsHTMLEditUtils::IsMozDiv(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  return aNode->Tag() == nsGkAtoms::div &&
         nsTextEditUtils::HasMozAttr(GetAsDOMNode(aNode));
}





bool
nsHTMLEditUtils::IsMailCite(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null parent passed to nsHTMLEditUtils::IsMailCite");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsMailCite(node);
}

bool
nsHTMLEditUtils::IsMailCite(nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  
  if (aNode->IsElement() &&
      aNode->AsElement()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                      NS_LITERAL_STRING("cite"),
                                      eIgnoreCase)) {
    return true;
  }

  
  if (aNode->IsElement() &&
      aNode->AsElement()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozquote,
                                      NS_LITERAL_STRING("true"),
                                      eIgnoreCase)) {
    return true;
  }

  return false;
}





bool
nsHTMLEditUtils::IsFormWidget(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditUtils::IsFormWidget");
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  return node && IsFormWidget(node);
}

bool
nsHTMLEditUtils::IsFormWidget(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  nsCOMPtr<nsIAtom> nodeAtom = aNode->Tag();
  return (nodeAtom == nsGkAtoms::textarea)
      || (nodeAtom == nsGkAtoms::select)
      || (nodeAtom == nsGkAtoms::button)
      || (nodeAtom == nsGkAtoms::output)
      || (nodeAtom == nsGkAtoms::keygen)
      || (nodeAtom == nsGkAtoms::progress)
      || (nodeAtom == nsGkAtoms::meter)
      || (nodeAtom == nsGkAtoms::input);
}

bool
nsHTMLEditUtils::SupportsAlignAttr(nsIDOMNode* aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditUtils::SupportsAlignAttr");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aNode);
  return (nodeAtom == nsGkAtoms::hr)
      || (nodeAtom == nsGkAtoms::table)
      || (nodeAtom == nsGkAtoms::tbody)
      || (nodeAtom == nsGkAtoms::tfoot)
      || (nodeAtom == nsGkAtoms::thead)
      || (nodeAtom == nsGkAtoms::tr)
      || (nodeAtom == nsGkAtoms::td)
      || (nodeAtom == nsGkAtoms::th)
      || (nodeAtom == nsGkAtoms::div)
      || (nodeAtom == nsGkAtoms::p)
      || (nodeAtom == nsGkAtoms::h1)
      || (nodeAtom == nsGkAtoms::h2)
      || (nodeAtom == nsGkAtoms::h3)
      || (nodeAtom == nsGkAtoms::h4)
      || (nodeAtom == nsGkAtoms::h5)
      || (nodeAtom == nsGkAtoms::h6);
}











#define GROUP_NONE             0


#define GROUP_TOPLEVEL         (1 << 1)


#define GROUP_HEAD_CONTENT     (1 << 2)


#define GROUP_FONTSTYLE        (1 << 3)



#define GROUP_PHRASE           (1 << 4)



#define GROUP_SPECIAL          (1 << 5)


#define GROUP_FORMCONTROL      (1 << 6)





#define GROUP_BLOCK            (1 << 7)


#define GROUP_FRAME            (1 << 8)


#define GROUP_TABLE_CONTENT    (1 << 9)


#define GROUP_TBODY_CONTENT    (1 << 10)


#define GROUP_TR_CONTENT       (1 << 11)


#define GROUP_COLGROUP_CONTENT (1 << 12)


#define GROUP_OBJECT_CONTENT   (1 << 13)


#define GROUP_LI               (1 << 14)


#define GROUP_MAP_CONTENT      (1 << 15)


#define GROUP_SELECT_CONTENT   (1 << 16)


#define GROUP_OPTIONS          (1 << 17)


#define GROUP_DL_CONTENT       (1 << 18)


#define GROUP_P                (1 << 19)


#define GROUP_LEAF             (1 << 20)



#define GROUP_OL_UL            (1 << 21)


#define GROUP_HEADING          (1 << 22)


#define GROUP_FIGCAPTION       (1 << 23)


#define GROUP_PICTURE_CONTENT  (1 << 24)

#define GROUP_INLINE_ELEMENT \
  (GROUP_FONTSTYLE | GROUP_PHRASE | GROUP_SPECIAL | GROUP_FORMCONTROL | \
   GROUP_LEAF)

#define GROUP_FLOW_ELEMENT (GROUP_INLINE_ELEMENT | GROUP_BLOCK)

struct nsElementInfo
{
#ifdef DEBUG
  eHTMLTags mTag;
#endif
  uint32_t mGroup;
  uint32_t mCanContainGroups;
  bool mIsContainer;
  bool mCanContainSelf;
};

#ifdef DEBUG
#define ELEM(_tag, _isContainer, _canContainSelf, _group, _canContainGroups) \
  { eHTMLTag_##_tag, _group, _canContainGroups, _isContainer, _canContainSelf }
#else
#define ELEM(_tag, _isContainer, _canContainSelf, _group, _canContainGroups) \
  { _group, _canContainGroups, _isContainer, _canContainSelf }
#endif

static const nsElementInfo kElements[eHTMLTag_userdefined] = {
  ELEM(a, true, false, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(abbr, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(acronym, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(address, true, true, GROUP_BLOCK,
       GROUP_INLINE_ELEMENT | GROUP_P),
  ELEM(applet, true, true, GROUP_SPECIAL | GROUP_BLOCK,
       GROUP_FLOW_ELEMENT | GROUP_OBJECT_CONTENT),
  ELEM(area, false, false, GROUP_MAP_CONTENT, GROUP_NONE),
  ELEM(article, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(aside, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(audio, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(b, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(base, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
  ELEM(basefont, false, false, GROUP_SPECIAL, GROUP_NONE),
  ELEM(bdo, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(bgsound, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(big, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(blockquote, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(body, true, true, GROUP_TOPLEVEL, GROUP_FLOW_ELEMENT),
  ELEM(br, false, false, GROUP_SPECIAL, GROUP_NONE),
  ELEM(button, true, true, GROUP_FORMCONTROL | GROUP_BLOCK,
       GROUP_FLOW_ELEMENT),
  ELEM(canvas, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(caption, true, true, GROUP_NONE, GROUP_INLINE_ELEMENT),
  ELEM(center, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(cite, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(code, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(col, false, false, GROUP_TABLE_CONTENT | GROUP_COLGROUP_CONTENT,
       GROUP_NONE),
  ELEM(colgroup, true, false, GROUP_NONE, GROUP_COLGROUP_CONTENT),
  ELEM(content, true, false, GROUP_NONE, GROUP_INLINE_ELEMENT),
  ELEM(data, true, false, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(datalist, true, false, GROUP_PHRASE,
       GROUP_OPTIONS | GROUP_INLINE_ELEMENT),
  ELEM(dd, true, false, GROUP_DL_CONTENT, GROUP_FLOW_ELEMENT),
  ELEM(del, true, true, GROUP_PHRASE | GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(dfn, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(dir, true, false, GROUP_BLOCK, GROUP_LI),
  ELEM(div, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(dl, true, false, GROUP_BLOCK, GROUP_DL_CONTENT),
  ELEM(dt, true, true, GROUP_DL_CONTENT, GROUP_INLINE_ELEMENT),
  ELEM(em, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(embed, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(fieldset, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(figcaption, true, false, GROUP_FIGCAPTION, GROUP_FLOW_ELEMENT),
  ELEM(figure, true, true, GROUP_BLOCK,
       GROUP_FLOW_ELEMENT | GROUP_FIGCAPTION),
  ELEM(font, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(footer, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(form, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(frame, false, false, GROUP_FRAME, GROUP_NONE),
  ELEM(frameset, true, true, GROUP_FRAME, GROUP_FRAME),
  ELEM(h1, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(h2, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(h3, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(h4, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(h5, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(h6, true, false, GROUP_BLOCK | GROUP_HEADING,
       GROUP_INLINE_ELEMENT),
  ELEM(head, true, false, GROUP_TOPLEVEL, GROUP_HEAD_CONTENT),
  ELEM(header, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(hgroup, true, false, GROUP_BLOCK, GROUP_HEADING),
  ELEM(hr, false, false, GROUP_BLOCK, GROUP_NONE),
  ELEM(html, true, false, GROUP_TOPLEVEL, GROUP_TOPLEVEL),
  ELEM(i, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(iframe, true, true, GROUP_SPECIAL | GROUP_BLOCK,
       GROUP_FLOW_ELEMENT),
  ELEM(image, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(img, false, false, GROUP_SPECIAL | GROUP_PICTURE_CONTENT, GROUP_NONE),
  ELEM(input, false, false, GROUP_FORMCONTROL, GROUP_NONE),
  ELEM(ins, true, true, GROUP_PHRASE | GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(kbd, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(keygen, false, false, GROUP_FORMCONTROL, GROUP_NONE),
  ELEM(label, true, false, GROUP_FORMCONTROL, GROUP_INLINE_ELEMENT),
  ELEM(legend, true, true, GROUP_NONE, GROUP_INLINE_ELEMENT),
  ELEM(li, true, false, GROUP_LI, GROUP_FLOW_ELEMENT),
  ELEM(link, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
  ELEM(listing, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(main, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(map, true, true, GROUP_SPECIAL, GROUP_BLOCK | GROUP_MAP_CONTENT),
  ELEM(mark, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(marquee, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(menu, true, true, GROUP_BLOCK, GROUP_LI | GROUP_FLOW_ELEMENT),
  ELEM(menuitem, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(meta, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
  ELEM(meter, true, false, GROUP_SPECIAL, GROUP_FLOW_ELEMENT),
  ELEM(multicol, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(nav, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(nobr, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(noembed, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(noframes, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(noscript, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(object, true, true, GROUP_SPECIAL | GROUP_BLOCK,
       GROUP_FLOW_ELEMENT | GROUP_OBJECT_CONTENT),
  
  ELEM(ol, true, true, GROUP_BLOCK | GROUP_OL_UL,
       GROUP_LI | GROUP_OL_UL),
  ELEM(optgroup, true, false, GROUP_SELECT_CONTENT,
       GROUP_OPTIONS),
  ELEM(option, true, false,
       GROUP_SELECT_CONTENT | GROUP_OPTIONS, GROUP_LEAF),
  ELEM(output, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(p, true, false, GROUP_BLOCK | GROUP_P, GROUP_INLINE_ELEMENT),
  ELEM(param, false, false, GROUP_OBJECT_CONTENT, GROUP_NONE),
  ELEM(picture, true, false, GROUP_SPECIAL, GROUP_PICTURE_CONTENT),
  ELEM(plaintext, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(pre, true, true, GROUP_BLOCK, GROUP_INLINE_ELEMENT),
  ELEM(progress, true, false, GROUP_SPECIAL, GROUP_FLOW_ELEMENT),
  ELEM(q, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(rb, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(rp, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(rt, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(rtc, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(ruby, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(s, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(samp, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(script, true, false, GROUP_HEAD_CONTENT | GROUP_SPECIAL,
       GROUP_LEAF),
  ELEM(section, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(select, true, false, GROUP_FORMCONTROL, GROUP_SELECT_CONTENT),
  ELEM(shadow, true, false, GROUP_NONE, GROUP_INLINE_ELEMENT),
  ELEM(small, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(source, false, false, GROUP_PICTURE_CONTENT, GROUP_NONE),
  ELEM(span, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(strike, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(strong, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(style, true, false, GROUP_HEAD_CONTENT, GROUP_LEAF),
  ELEM(sub, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(sup, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(table, true, false, GROUP_BLOCK, GROUP_TABLE_CONTENT),
  ELEM(tbody, true, false, GROUP_TABLE_CONTENT, GROUP_TBODY_CONTENT),
  ELEM(td, true, false, GROUP_TR_CONTENT, GROUP_FLOW_ELEMENT),
  ELEM(textarea, true, false, GROUP_FORMCONTROL, GROUP_LEAF),
  ELEM(tfoot, true, false, GROUP_NONE, GROUP_TBODY_CONTENT),
  ELEM(th, true, false, GROUP_TR_CONTENT, GROUP_FLOW_ELEMENT),
  ELEM(thead, true, false, GROUP_NONE, GROUP_TBODY_CONTENT),
  ELEM(template, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(time, true, false, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(title, true, false, GROUP_HEAD_CONTENT, GROUP_LEAF),
  ELEM(tr, true, false, GROUP_TBODY_CONTENT, GROUP_TR_CONTENT),
  ELEM(track, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(tt, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(u, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  
  ELEM(ul, true, true, GROUP_BLOCK | GROUP_OL_UL,
       GROUP_LI | GROUP_OL_UL),
  ELEM(var, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(video, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(wbr, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(xmp, false, false, GROUP_NONE, GROUP_NONE),

  
  ELEM(text, false, false, GROUP_LEAF, GROUP_NONE),
  ELEM(whitespace, false, false, GROUP_LEAF, GROUP_NONE),
  ELEM(newline, false, false, GROUP_LEAF, GROUP_NONE),
  ELEM(comment, false, false, GROUP_LEAF, GROUP_NONE),
  ELEM(entity, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(doctypeDecl, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(markupDecl, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(instruction, false, false, GROUP_NONE, GROUP_NONE),

  ELEM(userdefined, true, false, GROUP_NONE, GROUP_FLOW_ELEMENT)
};

bool
nsHTMLEditUtils::CanContain(int32_t aParent, int32_t aChild)
{
  NS_ASSERTION(aParent > eHTMLTag_unknown && aParent <= eHTMLTag_userdefined,
               "aParent out of range!");
  NS_ASSERTION(aChild > eHTMLTag_unknown && aChild <= eHTMLTag_userdefined,
               "aChild out of range!");

#ifdef DEBUG
  static bool checked = false;
  if (!checked) {
    checked = true;
    int32_t i;
    for (i = 1; i <= eHTMLTag_userdefined; ++i) {
      NS_ASSERTION(kElements[i - 1].mTag == i,
                   "You need to update kElements (missing tags).");
    }
  }
#endif

  
  if (aParent == eHTMLTag_button) {
    static const eHTMLTags kButtonExcludeKids[] = {
      eHTMLTag_a,
      eHTMLTag_fieldset,
      eHTMLTag_form,
      eHTMLTag_iframe,
      eHTMLTag_input,
      eHTMLTag_select,
      eHTMLTag_textarea
    };

    uint32_t j;
    for (j = 0; j < ArrayLength(kButtonExcludeKids); ++j) {
      if (kButtonExcludeKids[j] == aChild) {
        return false;
      }
    }
  }

  
  if (aChild == eHTMLTag_bgsound) {
    return false;
  }

  
  if (aChild == eHTMLTag_userdefined) {
    return true;
  }

  const nsElementInfo& parent = kElements[aParent - 1];
  if (aParent == aChild) {
    return parent.mCanContainSelf;
  }

  const nsElementInfo& child = kElements[aChild - 1];
  return (parent.mCanContainGroups & child.mGroup) != 0;
} 

bool
nsHTMLEditUtils::IsContainer(int32_t aTag)
{
  NS_ASSERTION(aTag > eHTMLTag_unknown && aTag <= eHTMLTag_userdefined,
               "aTag out of range!");

  return kElements[aTag - 1].mIsContainer;
}
