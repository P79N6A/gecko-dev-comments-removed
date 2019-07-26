





#ifndef mozilla_dom_quota_originorpatternstring_h__
#define mozilla_dom_quota_originorpatternstring_h__

#include "mozilla/dom/quota/QuotaCommon.h"

BEGIN_QUOTA_NAMESPACE

class OriginOrPatternString : public nsCString
{
public:
  static OriginOrPatternString
  FromOrigin(const nsACString& aOrigin)
  {
    return OriginOrPatternString(aOrigin, true);
  }

  static OriginOrPatternString
  FromPattern(const nsACString& aPattern)
  {
    return OriginOrPatternString(aPattern, false);
  }

  static OriginOrPatternString
  FromNull()
  {
    return OriginOrPatternString();
  }

  bool
  IsOrigin() const
  {
    return mIsOrigin;
  }

  bool
  IsPattern() const
  {
    return mIsPattern;
  }

  bool
  IsNull() const
  {
    return mIsNull;
  }

private:
  OriginOrPatternString(const nsACString& aOriginOrPattern, bool aIsOrigin)
  : nsCString(aOriginOrPattern),
    mIsOrigin(aIsOrigin), mIsPattern(!aIsOrigin), mIsNull(false)
  { }

  OriginOrPatternString()
  : mIsOrigin(false), mIsPattern(false), mIsNull(true)
  { }

  bool
  operator==(const OriginOrPatternString& aOther) MOZ_DELETE;

  bool mIsOrigin;
  bool mIsPattern;
  bool mIsNull;
};

END_QUOTA_NAMESPACE

#endif 
