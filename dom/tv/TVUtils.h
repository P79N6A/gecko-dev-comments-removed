





#ifndef mozilla_dom_TVUtils_h
#define mozilla_dom_TVUtils_h


#include "mozilla/dom/TVChannelBinding.h"

#include "mozilla/dom/TVSourceBinding.h"

namespace mozilla {
namespace dom {

inline nsString
ToTVSourceTypeStr(const TVSourceType aSourceType)
{
  MOZ_ASSERT(uint32_t(aSourceType) < ArrayLength(TVSourceTypeValues::strings));

  nsString str;
  str.AssignASCII(TVSourceTypeValues::strings[uint32_t(aSourceType)].value);
  return str;
}

inline TVSourceType
ToTVSourceType(const nsAString& aStr)
{
  if (aStr.EqualsLiteral("dvb-t")) {
    return TVSourceType::Dvb_t;
  }

  if (aStr.EqualsLiteral("dvb-t2")) {
    return TVSourceType::Dvb_t2;
  }

  if (aStr.EqualsLiteral("dvb-c")) {
    return TVSourceType::Dvb_c;
  }

  if (aStr.EqualsLiteral("dvb-c2")) {
    return TVSourceType::Dvb_c2;
  }

  if (aStr.EqualsLiteral("dvb-s")) {
    return TVSourceType::Dvb_s;
  }

  if (aStr.EqualsLiteral("dvb-s2")) {
    return TVSourceType::Dvb_s2;
  }

  if (aStr.EqualsLiteral("dvb-h")) {
    return TVSourceType::Dvb_h;
  }

  if (aStr.EqualsLiteral("dvb-sh")) {
    return TVSourceType::Dvb_sh;
  }

  if (aStr.EqualsLiteral("atsc")) {
    return TVSourceType::Atsc;
  }

  if (aStr.EqualsLiteral("atsc-m/h")) {
    return TVSourceType::Atsc_m_h;
  }

  if (aStr.EqualsLiteral("isdb-t")) {
    return TVSourceType::Isdb_t;
  }

  if (aStr.EqualsLiteral("isdb-tb")) {
    return TVSourceType::Isdb_tb;
  }

  if (aStr.EqualsLiteral("isdb-s")) {
    return TVSourceType::Isdb_s;
  }

  if (aStr.EqualsLiteral("isdb-c")) {
    return TVSourceType::Isdb_c;
  }

  if (aStr.EqualsLiteral("1seg")) {
    return TVSourceType::_1seg;
  }

  if (aStr.EqualsLiteral("dtmb")) {
    return TVSourceType::Dtmb;
  }

  if (aStr.EqualsLiteral("cmmb")) {
    return TVSourceType::Cmmb;
  }

  if (aStr.EqualsLiteral("t-dmb")) {
    return TVSourceType::T_dmb;
  }

  if (aStr.EqualsLiteral("s-dmb")) {
    return TVSourceType::S_dmb;
  }

  return TVSourceType::EndGuard_;
}

inline TVSourceType
ToTVSourceType(const char* aStr)
{
  nsString str;
  str.AssignASCII(aStr);
  return ToTVSourceType(str);
}

inline TVChannelType
ToTVChannelType(const nsAString& aStr)
{
  if (aStr.EqualsLiteral("tv")) {
    return TVChannelType::Tv;
  }

  if (aStr.EqualsLiteral("radio")) {
    return TVChannelType::Radio;
  }

  if (aStr.EqualsLiteral("data")) {
    return TVChannelType::Data;
  }

  return TVChannelType::EndGuard_;
}

} 
} 

#endif 
