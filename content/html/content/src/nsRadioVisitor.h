




































#ifndef _nsRadioVisitor_h__
#define _nsRadioVisitor_h__

#include "nsIRadioVisitor.h"

class nsIFormControl;
class nsIDocument;






class nsRadioVisitor : public nsIRadioVisitor
{
public:
  nsRadioVisitor() { }
  virtual ~nsRadioVisitor() { }

  NS_DECL_ISUPPORTS

  virtual PRBool Visit(nsIFormControl* aRadio) = 0;
};









class nsRadioSetCheckedChangedVisitor : public nsRadioVisitor
{
public:
  nsRadioSetCheckedChangedVisitor(bool aCheckedChanged)
    : mCheckedChanged(aCheckedChanged)
    { }

  virtual PRBool Visit(nsIFormControl* aRadio);

protected:
  bool mCheckedChanged;
};






class nsRadioGetCheckedChangedVisitor : public nsRadioVisitor
{
public:
  nsRadioGetCheckedChangedVisitor(bool* aCheckedChanged,
                                  nsIFormControl* aExcludeElement)
    : mCheckedChanged(aCheckedChanged)
    , mExcludeElement(aExcludeElement)
    { }

  virtual PRBool Visit(nsIFormControl* aRadio);

protected:
  bool* mCheckedChanged;
  nsIFormControl* mExcludeElement;
};






class nsRadioSetValueMissingState : public nsRadioVisitor
{
public:
  nsRadioSetValueMissingState(nsIFormControl* aExcludeElement,
                              bool aValidity, bool aNotify)
    : mExcludeElement(aExcludeElement)
    , mValidity(aValidity)
    , mNotify(aNotify)
    { }

  virtual PRBool Visit(nsIFormControl* aRadio);

protected:
  nsIFormControl* mExcludeElement;
  bool mValidity;
  bool mNotify;
};

#endif 

