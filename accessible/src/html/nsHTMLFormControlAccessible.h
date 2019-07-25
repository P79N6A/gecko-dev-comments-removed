





































#ifndef _nsHTMLFormControlAccessible_H_
#define _nsHTMLFormControlAccessible_H_

#include "nsFormControlAccessible.h"
#include "nsHyperTextAccessibleWrap.h"




typedef ProgressMeterAccessible<1> HTMLProgressMeterAccessible;




class nsHTMLCheckboxAccessible : public nsFormControlAccessible
{

public:
  enum { eAction_Click = 0 };

  nsHTMLCheckboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
};





class nsHTMLRadioButtonAccessible : public nsRadioButtonAccessible
{

public:
  nsHTMLRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint64 NativeState();
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
};






class nsHTMLButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTMLButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
};





class nsHTML4ButtonAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTML4ButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
};





class nsHTMLTextFieldAccessible : public nsHyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  nsHTMLTextFieldAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetValue(nsAString& _retval); 
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  NS_IMETHOD GetAssociatedEditor(nsIEditor **aEditor);

  
  virtual void ApplyARIAState(PRUint64* aState);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
  virtual nsAccessible* ContainerWidget() const;
};





class nsHTMLFileInputAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLFileInputAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  
  virtual PRUint32 NativeRole();
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);
};




class nsHTMLGroupboxAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLGroupboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual Relation RelationByType(PRUint32 aType);

protected:
  nsIContent* GetLegend();
};





class nsHTMLLegendAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLegendAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
  virtual Relation RelationByType(PRUint32 aType);
};




class nsHTMLFigureAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLFigureAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual Relation RelationByType(PRUint32 aType);

protected:
  nsIContent* Caption() const;
};





class nsHTMLFigcaptionAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLFigcaptionAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  
  virtual PRUint32 NativeRole();
  virtual Relation RelationByType(PRUint32 aType);
};

#endif
