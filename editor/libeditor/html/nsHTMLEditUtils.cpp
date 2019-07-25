




































#include "nsHTMLEditUtils.h"

#include "mozilla/Assertions.h"
#include "mozilla/Util.h"
#include "mozilla/dom/Element.h"

#include "nsTextEditUtils.h"

#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsEditor.h"
#include "nsEditProperty.h"
#include "nsIAtom.h"
#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsHTMLTags.h"

using namespace mozilla;



bool 
nsHTMLEditUtils::IsBig(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::big);
}





bool 
nsHTMLEditUtils::IsInlineStyle(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsInlineStyle");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::b)
      || (nodeAtom == nsEditProperty::i)
      || (nodeAtom == nsEditProperty::u)
      || (nodeAtom == nsEditProperty::tt)
      || (nodeAtom == nsEditProperty::s)
      || (nodeAtom == nsEditProperty::strike)
      || (nodeAtom == nsEditProperty::big)
      || (nodeAtom == nsEditProperty::small)
      || (nodeAtom == nsEditProperty::blink)
      || (nodeAtom == nsEditProperty::sub)
      || (nodeAtom == nsEditProperty::sup)
      || (nodeAtom == nsEditProperty::font);
}




bool
nsHTMLEditUtils::IsFormatNode(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsFormatNode");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::p)
      || (nodeAtom == nsEditProperty::pre)
      || (nodeAtom == nsEditProperty::h1)
      || (nodeAtom == nsEditProperty::h2)
      || (nodeAtom == nsEditProperty::h3)
      || (nodeAtom == nsEditProperty::h4)
      || (nodeAtom == nsEditProperty::h5)
      || (nodeAtom == nsEditProperty::h6)
      || (nodeAtom == nsEditProperty::address);
}




bool
nsHTMLEditUtils::IsNodeThatCanOutdent(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsNodeThatCanOutdent");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::ul)
      || (nodeAtom == nsEditProperty::ol)
      || (nodeAtom == nsEditProperty::dl)
      || (nodeAtom == nsEditProperty::li)
      || (nodeAtom == nsEditProperty::dd)
      || (nodeAtom == nsEditProperty::dt)
      || (nodeAtom == nsEditProperty::blockquote);
}



bool 
nsHTMLEditUtils::IsSmall(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::small);
}





 



bool 
nsHTMLEditUtils::IsHeader(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsHeader");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::h1)
      || (nodeAtom == nsEditProperty::h2)
      || (nodeAtom == nsEditProperty::h3)
      || (nodeAtom == nsEditProperty::h4)
      || (nodeAtom == nsEditProperty::h5)
      || (nodeAtom == nsEditProperty::h6);
}





bool 
nsHTMLEditUtils::IsParagraph(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::p);
}





bool 
nsHTMLEditUtils::IsHR(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::hr);
}





bool 
nsHTMLEditUtils::IsListItem(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsListItem");
  nsCOMPtr<dom::Element> element = do_QueryInterface(node);
  return element && IsListItem(element);
}

bool
nsHTMLEditUtils::IsListItem(dom::Element* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsEditProperty::li)
      || (nodeAtom == nsEditProperty::dd)
      || (nodeAtom == nsEditProperty::dt);
}





bool
nsHTMLEditUtils::IsTableElement(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null node passed to nsHTMLEditor::IsTableElement");
  nsCOMPtr<dom::Element> element = do_QueryInterface(node);
  return element && IsTableElement(element);
}

bool
nsHTMLEditUtils::IsTableElement(dom::Element* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsEditProperty::table)
      || (nodeAtom == nsEditProperty::tr)
      || (nodeAtom == nsEditProperty::td)
      || (nodeAtom == nsEditProperty::th)
      || (nodeAtom == nsEditProperty::thead)
      || (nodeAtom == nsEditProperty::tfoot)
      || (nodeAtom == nsEditProperty::tbody)
      || (nodeAtom == nsEditProperty::caption);
}




bool 
nsHTMLEditUtils::IsTableElementButNotTable(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null node passed to nsHTMLEditor::IsTableElementButNotTable");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::tr)
      || (nodeAtom == nsEditProperty::td)
      || (nodeAtom == nsEditProperty::th)
      || (nodeAtom == nsEditProperty::thead)
      || (nodeAtom == nsEditProperty::tfoot)
      || (nodeAtom == nsEditProperty::tbody)
      || (nodeAtom == nsEditProperty::caption);
}




bool 
nsHTMLEditUtils::IsTable(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::table);
}




bool 
nsHTMLEditUtils::IsTableRow(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::tr);
}





bool 
nsHTMLEditUtils::IsTableCell(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsTableCell");
  nsCOMPtr<dom::Element> element = do_QueryInterface(node);
  return element && IsTableCell(element);
}

bool
nsHTMLEditUtils::IsTableCell(dom::Element* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsEditProperty::td)
      || (nodeAtom == nsEditProperty::th);
}





bool 
nsHTMLEditUtils::IsTableCellOrCaption(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsTableCell");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(node);
  return (nodeAtom == nsEditProperty::td)
      || (nodeAtom == nsEditProperty::th)
      || (nodeAtom == nsEditProperty::caption);
}





bool
nsHTMLEditUtils::IsList(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsList");
  nsCOMPtr<dom::Element> element = do_QueryInterface(node);
  return element && IsList(element);
}

bool
nsHTMLEditUtils::IsList(dom::Element* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsEditProperty::ul)
      || (nodeAtom == nsEditProperty::ol)
      || (nodeAtom == nsEditProperty::dl);
}





bool 
nsHTMLEditUtils::IsOrderedList(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::ol);
}





bool 
nsHTMLEditUtils::IsUnorderedList(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::ul);
}





bool 
nsHTMLEditUtils::IsBlockquote(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::blockquote);
}





bool 
nsHTMLEditUtils::IsPre(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::pre);
}





bool 
nsHTMLEditUtils::IsImage(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::img);
}

bool 
nsHTMLEditUtils::IsLink(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, false);
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor = do_QueryInterface(aNode);
  if (anchor)
  {
    nsAutoString tmpText;
    if (NS_SUCCEEDED(anchor->GetHref(tmpText)) && !tmpText.IsEmpty())
      return true;
  }
  return false;
}

bool 
nsHTMLEditUtils::IsNamedAnchor(nsIDOMNode *aNode)
{
  nsCOMPtr<dom::Element> element = do_QueryInterface(aNode);
  return element && IsNamedAnchor(element);
}

bool
nsHTMLEditUtils::IsNamedAnchor(dom::Element* aNode)
{
  MOZ_ASSERT(aNode);
  if (!aNode->IsHTML(nsGkAtoms::a)) {
    return false;
  }

  nsAutoString text;
  return aNode->GetAttr(kNameSpaceID_None, nsGkAtoms::name, text) &&
         !text.IsEmpty();
}





bool 
nsHTMLEditUtils::IsDiv(nsIDOMNode *node)
{
  return nsEditor::NodeIsType(node, nsEditProperty::div);
}





bool 
nsHTMLEditUtils::IsMozDiv(nsIDOMNode *node)
{
  if (IsDiv(node) && nsTextEditUtils::HasMozAttr(node)) return true;
  return false;
}






bool 
nsHTMLEditUtils::IsMailCite(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null parent passed to nsHTMLEditUtils::IsMailCite");
  nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(node);
  if (!elem) {
    return false;
  }
  nsAutoString attrName (NS_LITERAL_STRING("type")); 
  
  
  nsAutoString attrVal;
  nsresult res = elem->GetAttribute(attrName, attrVal);
  ToLowerCase(attrVal);
  if (NS_SUCCEEDED(res))
  {
    if (attrVal.EqualsLiteral("cite"))
      return true;
  }

  
  attrName.AssignLiteral("_moz_quote");
  res = elem->GetAttribute(attrName, attrVal);
  if (NS_SUCCEEDED(res))
  {
    ToLowerCase(attrVal);
    if (attrVal.EqualsLiteral("true"))
      return true;
  }

  return false;
}





bool
nsHTMLEditUtils::IsFormWidget(nsIDOMNode *node)
{
  NS_PRECONDITION(node, "null node passed to nsHTMLEditUtils::IsFormWidget");
  nsCOMPtr<dom::Element> element = do_QueryInterface(node);
  return element && IsFormWidget(element);
}

bool
nsHTMLEditUtils::IsFormWidget(dom::Element* node)
{
  MOZ_ASSERT(node);
  nsCOMPtr<nsIAtom> nodeAtom = node->Tag();
  return (nodeAtom == nsEditProperty::textarea)
      || (nodeAtom == nsEditProperty::select)
      || (nodeAtom == nsEditProperty::button)
      || (nodeAtom == nsEditProperty::output)
      || (nodeAtom == nsEditProperty::keygen)
      || (nodeAtom == nsEditProperty::progress)
      || (nodeAtom == nsEditProperty::input);
}

bool
nsHTMLEditUtils::SupportsAlignAttr(nsIDOMNode * aNode)
{
  NS_PRECONDITION(aNode, "null node passed to nsHTMLEditUtils::SupportsAlignAttr");
  nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aNode);
  return (nodeAtom == nsEditProperty::hr)
      || (nodeAtom == nsEditProperty::table)
      || (nodeAtom == nsEditProperty::tbody)
      || (nodeAtom == nsEditProperty::tfoot)
      || (nodeAtom == nsEditProperty::thead)
      || (nodeAtom == nsEditProperty::tr)
      || (nodeAtom == nsEditProperty::td)
      || (nodeAtom == nsEditProperty::th)
      || (nodeAtom == nsEditProperty::div)
      || (nodeAtom == nsEditProperty::p)
      || (nodeAtom == nsEditProperty::h1)
      || (nodeAtom == nsEditProperty::h2)
      || (nodeAtom == nsEditProperty::h3)
      || (nodeAtom == nsEditProperty::h4)
      || (nodeAtom == nsEditProperty::h5)
      || (nodeAtom == nsEditProperty::h6);
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

#define GROUP_INLINE_ELEMENT \
  (GROUP_FONTSTYLE | GROUP_PHRASE | GROUP_SPECIAL | GROUP_FORMCONTROL | \
   GROUP_LEAF)

#define GROUP_FLOW_ELEMENT (GROUP_INLINE_ELEMENT | GROUP_BLOCK)

struct nsElementInfo
{
#ifdef DEBUG
  eHTMLTags mTag;
#endif
  PRUint32 mGroup;
  PRUint32 mCanContainGroups;
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
#if defined(MOZ_MEDIA)
  ELEM(audio, false, false, GROUP_NONE, GROUP_NONE),
#endif
  ELEM(b, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(base, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
  ELEM(basefont, false, false, GROUP_SPECIAL, GROUP_NONE),
  ELEM(bdo, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(bgsound, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(big, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(blink, false, false, GROUP_NONE, GROUP_NONE),
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
  ELEM(img, false, false, GROUP_SPECIAL, GROUP_NONE),
  ELEM(input, false, false, GROUP_FORMCONTROL, GROUP_NONE),
  ELEM(ins, true, true, GROUP_PHRASE | GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(kbd, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(keygen, false, false, GROUP_FORMCONTROL, GROUP_NONE),
  ELEM(label, true, false, GROUP_FORMCONTROL, GROUP_INLINE_ELEMENT),
  ELEM(legend, true, true, GROUP_NONE, GROUP_INLINE_ELEMENT),
  ELEM(li, true, false, GROUP_LI, GROUP_FLOW_ELEMENT),
  ELEM(link, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
  ELEM(listing, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(map, true, true, GROUP_SPECIAL, GROUP_BLOCK | GROUP_MAP_CONTENT),
  ELEM(mark, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(marquee, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(menu, true, true, GROUP_BLOCK, GROUP_LI | GROUP_FLOW_ELEMENT),
  ELEM(menuitem, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(meta, false, false, GROUP_HEAD_CONTENT, GROUP_NONE),
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
  ELEM(plaintext, false, false, GROUP_NONE, GROUP_NONE),
  ELEM(pre, true, true, GROUP_BLOCK, GROUP_INLINE_ELEMENT),
  ELEM(progress, true, false, GROUP_SPECIAL, GROUP_FLOW_ELEMENT),
  ELEM(q, true, true, GROUP_SPECIAL, GROUP_INLINE_ELEMENT),
  ELEM(s, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(samp, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
  ELEM(script, true, false, GROUP_HEAD_CONTENT | GROUP_SPECIAL,
       GROUP_LEAF),
  ELEM(section, true, true, GROUP_BLOCK, GROUP_FLOW_ELEMENT),
  ELEM(select, true, false, GROUP_FORMCONTROL, GROUP_SELECT_CONTENT),
  ELEM(small, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
#if defined(MOZ_MEDIA)
  ELEM(source, false, false, GROUP_NONE, GROUP_NONE),
#endif
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
  ELEM(title, true, false, GROUP_HEAD_CONTENT, GROUP_LEAF),
  ELEM(tr, true, false, GROUP_TBODY_CONTENT, GROUP_TR_CONTENT),
  ELEM(tt, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  ELEM(u, true, true, GROUP_FONTSTYLE, GROUP_INLINE_ELEMENT),
  
  ELEM(ul, true, true, GROUP_BLOCK | GROUP_OL_UL,
       GROUP_LI | GROUP_OL_UL),
  ELEM(var, true, true, GROUP_PHRASE, GROUP_INLINE_ELEMENT),
#if defined(MOZ_MEDIA)
  ELEM(video, false, false, GROUP_NONE, GROUP_NONE),
#endif
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
nsHTMLEditUtils::CanContain(PRInt32 aParent, PRInt32 aChild)
{
  NS_ASSERTION(aParent > eHTMLTag_unknown && aParent <= eHTMLTag_userdefined,
               "aParent out of range!");
  NS_ASSERTION(aChild > eHTMLTag_unknown && aChild <= eHTMLTag_userdefined,
               "aChild out of range!");

#ifdef DEBUG
  static bool checked = false;
  if (!checked) {
    checked = true;
    PRInt32 i;
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

    PRUint32 j;
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
nsHTMLEditUtils::IsContainer(PRInt32 aTag)
{
  NS_ASSERTION(aTag > eHTMLTag_unknown && aTag <= eHTMLTag_userdefined,
               "aTag out of range!");

  return kElements[aTag - 1].mIsContainer;
}
