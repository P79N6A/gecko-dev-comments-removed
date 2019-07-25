






































#ifndef nsIXFormsUtilityService_h
#define nsIXFormsUtilityService_h

#include "nsISupports.h"

class nsIDOMNode;
class nsIDOMNodeList;
class nsIEditor;


#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif


#define NS_IXFORMSUTILITYSERVICE_IID_STR "cd3457b6-cb6a-496c-bdfa-6cfecb2bd5fb"
#define NS_IXFORMSUTILITYSERVICE_IID \
{ 0xcd3457b6, 0xcb6a, 0x496c, \
  { 0xbd, 0xfa, 0x6c, 0xfe, 0xcb, 0x2b, 0xd5, 0xfb } }






class NS_NO_VTABLE nsIXFormsUtilityService : public nsISupports {
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXFORMSUTILITYSERVICE_IID)

  enum {
    STATE_OUT_OF_RANGE,
    STATE_IN_RANGE,
    STATE_NOT_A_RANGE
  };

  



  NS_IMETHOD GetBuiltinTypeName(nsIDOMNode *aElement, nsAString& aName) = 0;

  


  NS_IMETHOD IsReadonly(nsIDOMNode *aElement, PRBool *aState) = 0;

  


  NS_IMETHOD IsRelevant(nsIDOMNode *aElement, PRBool *aState) = 0;

  


  NS_IMETHOD IsRequired(nsIDOMNode *aElement, PRBool *aState) = 0;

  


  NS_IMETHOD IsValid(nsIDOMNode *aElement, PRBool *aState) = 0;

  





  NS_IMETHOD IsInRange(nsIDOMNode *aElement, PRUint32 *aState) = 0;

  



  NS_IMETHOD GetValue(nsIDOMNode *aElement, nsAString& aValue) = 0;

  


  NS_IMETHOD Focus(nsIDOMNode *aElement) = 0;

  



  NS_IMETHOD GetRangeStart(nsIDOMNode *aElement, nsAString& aValue) = 0;

  



  NS_IMETHOD GetRangeEnd(nsIDOMNode *aElement, nsAString& aValue) = 0;

  



  NS_IMETHOD GetRangeStep(nsIDOMNode *aElement, nsAString& aValue) = 0;

  



  NS_IMETHOD GetEditor(nsIDOMNode *aElement, nsIEditor **aEditor) = 0;

  




  NS_IMETHOD IsDropmarkerOpen(nsIDOMNode *aElement, PRBool* aIsOpen) = 0;

  




  NS_IMETHOD ToggleDropmarkerState(nsIDOMNode *aElement) = 0;

  



  NS_IMETHOD GetSelectedItemForSelect1(nsIDOMNode *aElement,
                                       nsIDOMNode ** aItem) = 0;

  



  NS_IMETHOD SetSelectedItemForSelect1(nsIDOMNode *aElement,
                                       nsIDOMNode *aItem) = 0;

  



  NS_IMETHOD GetSelectedItemsForSelect(nsIDOMNode *aElement,
                                       nsIDOMNodeList **aItems) = 0;

  



  NS_IMETHOD AddItemToSelectionForSelect(nsIDOMNode *aElement,
                                         nsIDOMNode *Item) = 0;

  



  NS_IMETHOD RemoveItemFromSelectionForSelect(nsIDOMNode *aElement,
                                              nsIDOMNode *Item) = 0;

  



  NS_IMETHOD ClearSelectionForSelect(nsIDOMNode *aElement) = 0;

  



  NS_IMETHOD SelectAllItemsForSelect(nsIDOMNode *aElement) = 0;

  




  NS_IMETHOD IsSelectItemSelected(nsIDOMNode *aElement, nsIDOMNode *aItem,
                                  PRBool *aIsSelected) = 0;

  





  NS_IMETHOD GetSelectChildrenFor(nsIDOMNode *aElement,
                                  nsIDOMNodeList **aNodeList) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXFormsUtilityService,
                              NS_IXFORMSUTILITYSERVICE_IID)

#endif 
