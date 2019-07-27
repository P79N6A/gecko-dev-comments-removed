




#ifndef nsHTMLCSSUtils_h__
#define nsHTMLCSSUtils_h__

#include "ChangeStyleTxn.h"             
#include "nsCOMPtr.h"                   
#include "nsTArray.h"                   
#include "nscore.h"                     

class nsComputedDOMStyle;
class nsIAtom;
class nsIContent;
class nsIDOMCSSStyleDeclaration;
class nsIDOMElement;
class nsIDOMNode;
class nsINode;
class nsString;
namespace mozilla {
namespace dom {
class Element;
}  
}  

class nsHTMLEditor;
class nsIDOMWindow;

typedef void (*nsProcessValueFunc)(const nsAString * aInputString, nsAString & aOutputString,
                                   const char * aDefaultValueString,
                                   const char * aPrependString, const char* aAppendString);

class nsHTMLCSSUtils
{
public:
  explicit nsHTMLCSSUtils(nsHTMLEditor* aEditor);
  ~nsHTMLCSSUtils();

  enum nsCSSEditableProperty {
    eCSSEditableProperty_NONE=0,
    eCSSEditableProperty_background_color,
    eCSSEditableProperty_background_image,
    eCSSEditableProperty_border,
    eCSSEditableProperty_caption_side,
    eCSSEditableProperty_color,
    eCSSEditableProperty_float,
    eCSSEditableProperty_font_family,
    eCSSEditableProperty_font_size,
    eCSSEditableProperty_font_style,
    eCSSEditableProperty_font_weight,
    eCSSEditableProperty_height,
    eCSSEditableProperty_list_style_type,
    eCSSEditableProperty_margin_left,
    eCSSEditableProperty_margin_right,
    eCSSEditableProperty_text_align,
    eCSSEditableProperty_text_decoration,
    eCSSEditableProperty_vertical_align,
    eCSSEditableProperty_whitespace,
    eCSSEditableProperty_width
  };

  enum StyleType { eSpecified, eComputed };


  struct CSSEquivTable {
    nsCSSEditableProperty cssProperty;
    nsProcessValueFunc processValueFunctor;
    const char * defaultValue;
    const char * prependValue;
    const char * appendValue;
    bool gettable;
    bool caseSensitiveValue;
  };

  








  bool IsCSSEditableProperty(nsINode* aNode, nsIAtom* aProperty,
                             const nsAString* aAttribute);
  bool IsCSSEditableProperty(nsIDOMNode* aNode, nsIAtom* aProperty, const nsAString* aAttribute);

  







  nsresult SetCSSProperty(mozilla::dom::Element& aElement, nsIAtom& aProperty,
                          const nsAString& aValue, bool aSuppressTxn = false);
  nsresult SetCSSPropertyPixels(mozilla::dom::Element& aElement,
                                nsIAtom& aProperty, int32_t aIntValue);
  nsresult RemoveCSSProperty(mozilla::dom::Element& aElement,
                             nsIAtom& aProperty,
                             const nsAString& aPropertyValue,
                             bool aSuppressTxn = false);

  






  nsresult    SetCSSProperty(nsIDOMElement * aElement,
                             const nsAString & aProperty,
                             const nsAString & aValue);
  nsresult    SetCSSPropertyPixels(nsIDOMElement * aElement,
                                   const nsAString & aProperty,
                                   int32_t aIntValue);

  






  nsresult    GetSpecifiedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                   nsAString & aValue);
  nsresult    GetComputedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                  nsAString & aValue);

  







  nsresult    RemoveCSSInlineStyle(nsIDOMNode * aNode, nsIAtom * aProperty, const nsAString & aPropertyValue);

   






  bool        IsCSSInvertable(nsIAtom * aProperty, const nsAString * aAttribute);

  



  void        GetDefaultBackgroundColor(nsAString & aColor);

  



  void        GetDefaultLengthUnit(nsAString & aLengthUnit);

  








  nsresult    GetCSSEquivalentToHTMLInlineStyleSet(nsINode* aNode,
                                                   nsIAtom * aHTMLProperty,
                                                   const nsAString * aAttribute,
                                                   nsAString & aValueString,
                                                   StyleType aStyleType);

  











  bool IsCSSEquivalentToHTMLInlineStyleSet(nsINode* aContent,
                                           nsIAtom* aProperty,
                                           const nsAString* aAttribute,
                                           const nsAString& aValue,
                                           StyleType aStyleType);

  nsresult    IsCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                  nsIAtom * aHTMLProperty,
                                                  const nsAString * aAttribute,
                                                  bool & aIsSet,
                                                  nsAString & aValueString,
                                                  StyleType aStyleType);

  













  int32_t     SetCSSEquivalentToHTMLStyle(mozilla::dom::Element* aElement,
                                          nsIAtom* aProperty,
                                          const nsAString* aAttribute,
                                          const nsAString* aValue,
                                          bool aSuppressTransaction);
  nsresult    SetCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                          nsIAtom * aHTMLProperty,
                                          const nsAString * aAttribute,
                                          const nsAString * aValue,
                                          int32_t * aCount,
                                          bool aSuppressTransaction);

  








  nsresult    RemoveCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                             nsIAtom *aHTMLProperty,
                                             const nsAString *aAttribute,
                                             const nsAString *aValue,
                                             bool aSuppressTransaction);
  








  nsresult    RemoveCSSEquivalentToHTMLStyle(mozilla::dom::Element* aElement,
                                             nsIAtom* aHTMLProperty,
                                             const nsAString* aAttribute,
                                             const nsAString* aValue,
                                             bool aSuppressTransaction);

  







  void        ParseLength(const nsAString & aString, float * aValue, nsIAtom ** aUnit);

  




  void        SetCSSEnabled(bool aIsCSSPrefChecked);

  




  bool        IsCSSPrefChecked();

  







  bool ElementsSameStyle(mozilla::dom::Element* aFirstNode,
                         mozilla::dom::Element* aSecondNode);
  bool ElementsSameStyle(nsIDOMNode *aFirstNode, nsIDOMNode *aSecondNode);

  





  nsresult GetInlineStyles(mozilla::dom::Element* aElement,
                           nsIDOMCSSStyleDeclaration** aCssDecl,
                           uint32_t* aLength);
  nsresult GetInlineStyles(nsIDOMElement* aElement,
                           nsIDOMCSSStyleDeclaration** aCssDecl,
                           uint32_t* aLength);
private:
  nsresult GetInlineStyles(nsISupports* aElement,
                           nsIDOMCSSStyleDeclaration** aCssDecl,
                           uint32_t* aLength);

public:
  





  mozilla::dom::Element* GetElementContainerOrSelf(nsINode* aNode);
  already_AddRefed<nsIDOMElement> GetElementContainerOrSelf(nsIDOMNode* aNode);

  


  already_AddRefed<nsComputedDOMStyle>
    GetComputedStyle(nsIDOMElement* aElement);
  already_AddRefed<nsComputedDOMStyle>
    GetComputedStyle(mozilla::dom::Element* aElement);


private:

  




  void  GetCSSPropertyAtom(nsCSSEditableProperty aProperty, nsIAtom ** aAtom);

  











  void      BuildCSSDeclarations(nsTArray<nsIAtom*> & aPropertyArray,
                                 nsTArray<nsString> & cssValueArray,
                                 const CSSEquivTable * aEquivTable,
                                 const nsAString * aValue,
                                 bool aGetOrRemoveRequest);

  












  void      GenerateCSSDeclarationsFromHTMLStyle(mozilla::dom::Element* aNode,
                                                 nsIAtom* aHTMLProperty,
                                                 const nsAString* aAttribute,
                                                 const nsAString* aValue,
                                                 nsTArray<nsIAtom*>& aPropertyArray,
                                                 nsTArray<nsString>& aValueArray,
                                                 bool aGetOrRemoveRequest);

  







  already_AddRefed<mozilla::dom::ChangeStyleTxn>
  CreateCSSPropertyTxn(mozilla::dom::Element& aElement,
      nsIAtom& aProperty, const nsAString& aValue,
      mozilla::dom::ChangeStyleTxn::EChangeType aChangeType);

  






  nsresult GetCSSInlinePropertyBase(nsINode* aNode, nsIAtom* aProperty,
                                    nsAString& aValue, StyleType aStyleType);
  nsresult GetCSSInlinePropertyBase(nsIDOMNode* aNode, nsIAtom* aProperty,
                                    nsAString& aValue, StyleType aStyleType);


private:
  nsHTMLEditor            *mHTMLEditor;
  bool                    mIsCSSPrefChecked; 
};

#define NS_EDITOR_INDENT_INCREMENT_IN        0.4134f
#define NS_EDITOR_INDENT_INCREMENT_CM        1.05f
#define NS_EDITOR_INDENT_INCREMENT_MM        10.5f
#define NS_EDITOR_INDENT_INCREMENT_PT        29.76f
#define NS_EDITOR_INDENT_INCREMENT_PC        2.48f
#define NS_EDITOR_INDENT_INCREMENT_EM        3
#define NS_EDITOR_INDENT_INCREMENT_EX        6
#define NS_EDITOR_INDENT_INCREMENT_PX        40
#define NS_EDITOR_INDENT_INCREMENT_PERCENT   4 

#endif 
