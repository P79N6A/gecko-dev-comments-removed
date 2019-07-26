




#ifndef MOZILLA_A11Y_HTMLFormControlAccessible_H_
#define MOZILLA_A11Y_HTMLFormControlAccessible_H_

#include "FormControlAccessible.h"
#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {




typedef ProgressMeterAccessible<1> HTMLProgressMeterAccessible;




class HTMLCheckboxAccessible : public nsLeafAccessible
{

public:
  enum { eAction_Click = 0 };

  HTMLCheckboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
};





class HTMLRadioButtonAccessible : public RadioButtonAccessible
{

public:
  HTMLRadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual PRUint64 NativeState();
  virtual void GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                          PRInt32 *aSetSize);
};






class HTMLButtonAccessible : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLButtonAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
};





class HTMLTextFieldAccessible : public HyperTextAccessibleWrap
{

public:
  enum { eAction_Click = 0 };

  HTMLTextFieldAccessible(nsIContent* aContent, DocAccessible* aDoc);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual already_AddRefed<nsIEditor> GetEditor() const;

  
  virtual void Value(nsString& aValue);
  virtual void ApplyARIAState(PRUint64* aState) const;
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 State();
  virtual PRUint64 NativeState();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsWidget() const;
  virtual Accessible* ContainerWidget() const;
};





class HTMLFileInputAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFileInputAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual nsresult HandleAccEvent(AccEvent* aAccEvent);
};




class HTMLGroupboxAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLGroupboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aType);

protected:
  nsIContent* GetLegend();
};





class HTMLLegendAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLegendAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aType);
};




class HTMLFigureAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigureAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aType);

protected:
  nsIContent* Caption() const;
};





class HTMLFigcaptionAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLFigcaptionAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual Relation RelationByType(PRUint32 aType);
};

} 
} 

#endif
