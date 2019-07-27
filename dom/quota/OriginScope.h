





#ifndef mozilla_dom_quota_originorpatternstring_h__
#define mozilla_dom_quota_originorpatternstring_h__

#include "mozilla/dom/quota/QuotaCommon.h"

BEGIN_QUOTA_NAMESPACE

class OriginScope : public nsCString
{
public:
  enum Type
  {
    eOrigin,
    ePattern,
    eNull
  };

  static OriginScope
  FromOrigin(const nsACString& aOrigin)
  {
    return OriginScope(aOrigin, eOrigin);
  }

  static OriginScope
  FromPattern(const nsACString& aPattern)
  {
    return OriginScope(aPattern, ePattern);
  }

  static OriginScope
  FromNull()
  {
    return OriginScope(NullCString(), eNull);
  }

  bool
  IsOrigin() const
  {
    return mType == eOrigin;
  }

  bool
  IsPattern() const
  {
    return mType == ePattern;
  }

  bool
  IsNull() const
  {
    return mType == eNull;
  }

  Type
  GetType() const
  {
    return mType;
  }

private:
  OriginScope(const nsACString& aString, Type aType)
  : nsCString(aString), mType(aType)
  { }

  bool
  operator==(const OriginScope& aOther) = delete;

  const Type mType;
};

END_QUOTA_NAMESPACE

#endif
