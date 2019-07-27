





#ifndef mozilla_a11y_xpcAccessibleGeneric_h_
#define mozilla_a11y_xpcAccessibleGeneric_h_

#include "xpcAccessible.h"
#include "xpcAccessibleHyperLink.h"
#include "xpcAccessibleSelectable.h"
#include "xpcAccessibleValue.h"

#include "Accessible.h"

namespace mozilla {
namespace a11y {




class xpcAccessibleGeneric : public xpcAccessible,
                             public xpcAccessibleHyperLink,
                             public xpcAccessibleSelectable,
                             public xpcAccessibleValue
{
public:
  explicit xpcAccessibleGeneric(Accessible* aInternal) :
    mIntl(aInternal), mSupportedIfaces(0)
  {
    if (mIntl->IsSelect())
      mSupportedIfaces |= eSelectable;
    if (mIntl->HasNumericValue())
      mSupportedIfaces |= eValue;
    if (mIntl->IsLink())
      mSupportedIfaces |= eHyperLink;
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(xpcAccessibleGeneric, nsIAccessible)

  
  virtual Accessible* ToInternalAccessible() const final override;

  
  virtual void Shutdown();

protected:
  virtual ~xpcAccessibleGeneric() {}

  Accessible* mIntl;

  enum {
    eSelectable = 1 << 0,
    eValue = 1 << 1,
    eHyperLink = 1 << 2,
    eText = 1 << 3
  };
  uint8_t mSupportedIfaces;

private:
  friend class Accessible;
  friend class xpcAccessible;
  friend class xpcAccessibleHyperLink;
  friend class xpcAccessibleSelectable;
  friend class xpcAccessibleValue;

  xpcAccessibleGeneric(const xpcAccessibleGeneric&) = delete;
  xpcAccessibleGeneric& operator =(const xpcAccessibleGeneric&) = delete;
};

inline Accessible*
xpcAccessible::Intl()
{
  return static_cast<xpcAccessibleGeneric*>(this)->mIntl;
}

inline Accessible*
xpcAccessibleHyperLink::Intl()
{
  return static_cast<xpcAccessibleGeneric*>(this)->mIntl;
}

inline Accessible*
xpcAccessibleSelectable::Intl()
{
  return static_cast<xpcAccessibleGeneric*>(this)->mIntl;
}

inline Accessible*
xpcAccessibleValue::Intl()
{
  return static_cast<xpcAccessibleGeneric*>(this)->mIntl;
}

} 
} 

#endif
