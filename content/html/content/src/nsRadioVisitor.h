




#ifndef _nsRadioVisitor_h__
#define _nsRadioVisitor_h__

#include "mozilla/Attributes.h"
#include "nsIRadioVisitor.h"

class nsIFormControl;





class nsRadioVisitor : public nsIRadioVisitor
{
public:
  nsRadioVisitor() { }
  virtual ~nsRadioVisitor() { }

  NS_DECL_ISUPPORTS

  virtual bool Visit(nsIFormControl* aRadio) MOZ_OVERRIDE = 0;
};









class nsRadioSetCheckedChangedVisitor : public nsRadioVisitor
{
public:
  nsRadioSetCheckedChangedVisitor(bool aCheckedChanged)
    : mCheckedChanged(aCheckedChanged)
    { }

  virtual bool Visit(nsIFormControl* aRadio) MOZ_OVERRIDE;

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

  virtual bool Visit(nsIFormControl* aRadio) MOZ_OVERRIDE;

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

  virtual bool Visit(nsIFormControl* aRadio) MOZ_OVERRIDE;

protected:
  nsIFormControl* mExcludeElement;
  bool mValidity;
  bool mNotify;
};

#endif 

