





#ifndef mozilla_dom_cellbroadcast_CellBroadcastMessage_h
#define mozilla_dom_cellbroadcast_CellBroadcastMessage_h

#include "mozilla/dom/MozCellBroadcastMessageBinding.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsWrapperCache.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class CellBroadcastEtwsInfo;

class CellBroadcastMessage final : public nsISupports
                                 , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CellBroadcastMessage)

  CellBroadcastMessage(nsPIDOMWindow* aWindow,
                       uint32_t aServiceId,
                       uint32_t aGsmGeographicalScope,
                       uint16_t aMessageCode,
                       uint16_t aMessageId,
                       const nsAString& aLanguage,
                       const nsAString& aBody,
                       uint32_t aMessageClass,
                       uint64_t aTimestamp,
                       uint32_t aCdmaServiceCategory,
                       bool aHasEtwsInfo,
                       uint32_t aEtwsWarningType,
                       bool aEtwsEmergencyUserAlert,
                       bool aEtwsPopup);

  nsPIDOMWindow*
  GetParentObject() const { return mWindow; }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  

  uint32_t ServiceId() const { return mServiceId; }

  const Nullable<CellBroadcastGsmGeographicalScope>&
  GetGsmGeographicalScope() { return mGsmGeographicalScope; }

  uint16_t MessageCode() const { return mMessageCode; }

  uint16_t MessageId() const { return mMessageId; }

  void GetLanguage(nsString& aLanguage) const { aLanguage = mLanguage; }

  void GetBody(nsString& aBody) const { aBody = mBody; }

  const Nullable<CellBroadcastMessageClass>&
  GetMessageClass() { return mMessageClass; }

  uint64_t Timestamp() const { return mTimestamp; }

  
  already_AddRefed<CellBroadcastEtwsInfo> GetEtws() const;

  const Nullable<uint16_t>& GetCdmaServiceCategory() { return mCdmaServiceCategory; };

private:
  
  ~CellBroadcastMessage() {};

  
  CellBroadcastMessage();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  uint32_t mServiceId;
  Nullable<CellBroadcastGsmGeographicalScope> mGsmGeographicalScope;
  uint16_t mMessageCode;
  uint16_t mMessageId;
  nsString mLanguage;
  nsString mBody;
  Nullable<CellBroadcastMessageClass> mMessageClass;
  uint64_t mTimestamp;
  Nullable<uint16_t> mCdmaServiceCategory;
  nsRefPtr<CellBroadcastEtwsInfo> mEtwsInfo;
};

class CellBroadcastEtwsInfo final : public nsISupports
                                  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(CellBroadcastEtwsInfo)

  CellBroadcastEtwsInfo(nsPIDOMWindow* aWindow,
                        uint32_t aWarningType,
                        bool aEmergencyUserAlert,
                        bool aPopup);

  nsPIDOMWindow*
  GetParentObject() const { return mWindow; }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  

  const Nullable<CellBroadcastEtwsWarningType>&
  GetWarningType()  { return mWarningType; }

  bool EmergencyUserAlert() const { return mEmergencyUserAlert; }

  bool Popup() const { return mPopup; }

private:
  
  ~CellBroadcastEtwsInfo() {};

  
  CellBroadcastEtwsInfo();

  nsCOMPtr<nsPIDOMWindow> mWindow;
  Nullable<CellBroadcastEtwsWarningType> mWarningType;
  bool mEmergencyUserAlert;
  bool mPopup;
};

} 
} 

#endif 
