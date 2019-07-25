






































#ifndef nsHTMLCSSUtils_h__
#define nsHTMLCSSUtils_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIHTMLEditor.h"
#include "ChangeCSSInlineStyleTxn.h"
#include "nsEditProperty.h"
#include "nsIDOMCSSStyleDeclaration.h"

#define SPECIFIED_STYLE_TYPE    1
#define COMPUTED_STYLE_TYPE     2

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


  struct CSSEquivTable {
    nsCSSEditableProperty cssProperty;
    nsProcessValueFunc processValueFunctor;
    const char * defaultValue;
    const char * prependValue;
    const char * appendValue;
    PRBool gettable;
    PRBool caseSensitiveValue;
  };

  







  PRBool      IsCSSEditableProperty(nsIDOMNode * aNode, nsIAtom * aProperty, const nsAString * aAttribute);

  







  nsresult    SetCSSProperty(nsIDOMElement * aElement, nsIAtom * aProperty,
                             const nsAString & aValue,
                             PRBool aSuppressTransaction);
  nsresult    SetCSSPropertyPixels(nsIDOMElement *aElement, nsIAtom *aProperty,
                                   PRInt32 aIntValue, PRBool aSuppressTxn);
  nsresult    RemoveCSSProperty(nsIDOMElement * aElement, nsIAtom * aProperty,
                                const nsAString & aPropertyValue, PRBool aSuppressTransaction);

  






  nsresult    SetCSSProperty(nsIDOMElement * aElement,
                             const nsAString & aProperty,
                             const nsAString & aValue);
  nsresult    SetCSSPropertyPixels(nsIDOMElement * aElement,
                                   const nsAString & aProperty,
                                   PRInt32 aIntValue);
  nsresult    RemoveCSSProperty(nsIDOMElement * aElement,
                                const nsAString & aProperty);

  






  nsresult    GetSpecifiedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                   nsAString & aValue);
  nsresult    GetComputedProperty(nsIDOMNode *aNode, nsIAtom *aProperty,
                                  nsAString & aValue);

  







  nsresult    RemoveCSSInlineStyle(nsIDOMNode * aNode, nsIAtom * aProperty, const nsAString & aPropertyValue);

   






  PRBool      IsCSSInvertable(nsIAtom * aProperty, const nsAString * aAttribute);

  



  void        GetDefaultBackgroundColor(nsAString & aColor);

  



  void        GetDefaultLengthUnit(nsAString & aLengthUnit);

  




  nsresult    HasClassOrID(nsIDOMElement * aElement, PRBool & aReturn);

  









  nsresult    GetCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                   nsIAtom * aHTMLProperty,
                                                   const nsAString * aAttribute,
                                                   nsAString & aValueString,
                                                   PRUint8 aStyleType);

  










  nsresult    IsCSSEquivalentToHTMLInlineStyleSet(nsIDOMNode * aNode,
                                                  nsIAtom * aHTMLProperty,
                                                  const nsAString * aAttribute,
                                                  PRBool & aIsSet,
                                                  nsAString & aValueString,
                                                  PRUint8 aStyleType);

  










  nsresult    SetCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                          nsIAtom * aHTMLProperty,
                                          const nsAString * aAttribute,
                                          const nsAString * aValue,
                                          PRInt32 * aCount,
                                          PRBool aSuppressTransaction);

  








  nsresult    RemoveCSSEquivalentToHTMLStyle(nsIDOMNode * aNode,
                                             nsIAtom *aHTMLProperty,
                                             const nsAString *aAttribute,
                                             const nsAString *aValue,
                                             PRBool aSuppressTransaction);

  







  void        ParseLength(const nsAString & aString, float * aValue, nsIAtom ** aUnit);

  




  nsresult    SetCSSEnabled(PRBool aIsCSSPrefChecked);

  




  PRBool      IsCSSPrefChecked();

  







  PRBool ElementsSameStyle(nsIDOMNode *aFirstNode, nsIDOMNode *aSecondNode);

  





  nsresult GetInlineStyles(nsIDOMElement * aElement, nsIDOMCSSStyleDeclaration ** aCssDecl,
                           PRUint32 * aLength);

  





  nsresult GetElementContainerOrSelf(nsIDOMNode * aNode, nsIDOMElement ** aElement);

  





  nsresult        GetDefaultViewCSS(nsIDOMNode* aNode, nsIDOMWindow** aWindow);


private:

  




  void  GetCSSPropertyAtom(nsCSSEditableProperty aProperty, nsIAtom ** aAtom);

  











  void      BuildCSSDeclarations(nsTArray<nsIAtom*> & aPropertyArray,
                                 nsTArray<nsString> & cssValueArray,
                                 const CSSEquivTable * aEquivTable,
                                 const nsAString * aValue,
                                 PRBool aGetOrRemoveRequest);

  












  void      GenerateCSSDeclarationsFromHTMLStyle(nsIDOMNode * aNode,
                                                 nsIAtom * aHTMLProperty,
                                                 const nsAString *aAttribute,
                                                 const nsAString *aValue,
                                                 nsTArray<nsIAtom*> & aPropertyArray,
                                                 nsTArray<nsString> & aValueArray,
                                                 PRBool aGetOrRemoveRequest);

  







  nsresult    CreateCSSPropertyTxn(nsIDOMElement * aElement, 
                                   nsIAtom * aProperty,
                                   const nsAString & aValue,
                                   ChangeCSSInlineStyleTxn ** aTxn,
                                   PRBool aRemoveProperty);

  








  nsresult    GetCSSInlinePropertyBase(nsIDOMNode * aNode, nsIAtom * aProperty,
                                       nsAString & aValue,
                                       nsIDOMWindow* aWindow,
                                       PRUint8 aStyleType);


private:
  nsHTMLEditor            *mHTMLEditor;
  PRBool                  mIsCSSPrefChecked; 
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
