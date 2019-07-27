



#ifndef mozilla_dom_IccInfo_h
#define mozilla_dom_IccInfo_h

#include "MozIccInfoBinding.h"
#include "nsIIccInfo.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

namespace icc {
class IccInfoData;
} 

class IccInfo : public nsIIccInfo
              , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IccInfo)
  NS_DECL_NSIICCINFO

  explicit IccInfo(nsPIDOMWindow* aWindow);
  explicit IccInfo(const icc::IccInfoData& aData);

  void
  Update(nsIIccInfo* aInfo);

  nsPIDOMWindow*
  GetParentObject() const
  {
    return mWindow;
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  Nullable<IccType>
  GetIccType() const;

  void
  GetIccid(nsAString& aIccId) const;

  void
  GetMcc(nsAString& aMcc) const;

  void
  GetMnc(nsAString& aMnc) const;

  void
  GetSpn(nsAString& aSpn) const;

  bool
  IsDisplayNetworkNameRequired() const;

  bool
  IsDisplaySpnRequired() const;

protected:
  virtual ~IccInfo() {}

  nsCOMPtr<nsPIDOMWindow> mWindow;
  
  
  
  
  nsString mIccType;
  nsString mIccid;
  nsString mMcc;
  nsString mMnc;
  nsString mSpn;
  bool mIsDisplayNetworkNameRequired;
  bool mIsDisplaySpnRequired;
};

class GsmIccInfo MOZ_FINAL : public IccInfo
                           , public nsIGsmIccInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIICCINFO(IccInfo::)
  NS_DECL_NSIGSMICCINFO

  explicit GsmIccInfo(nsPIDOMWindow* aWindow);
  explicit GsmIccInfo(const icc::IccInfoData& aData);

  void
  Update(nsIGsmIccInfo* aInfo);

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  void
  GetMsisdn(nsAString& aMsisdn) const;

private:
  ~GsmIccInfo() {}

  nsString mPhoneNumber;
};

class CdmaIccInfo MOZ_FINAL : public IccInfo
                            , public nsICdmaIccInfo
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIICCINFO(IccInfo::)
  NS_DECL_NSICDMAICCINFO

  explicit CdmaIccInfo(nsPIDOMWindow* aWindow);
  explicit CdmaIccInfo(const icc::IccInfoData& aData);

  void
  Update(nsICdmaIccInfo* aInfo);

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  void
  GetMdn(nsAString& aMdn) const;

  int32_t
  PrlVersion() const;

private:
  ~CdmaIccInfo() {}

  nsString mPhoneNumber;
  int32_t mPrlVersion;
};

} 
} 

#endif 

