





#ifndef mozilla_dom_bluetooth_bluetoothreplyrunnable_h__
#define mozilla_dom_bluetooth_bluetoothreplyrunnable_h__

#include "mozilla/Attributes.h"
#include "BluetoothCommon.h"
#include "nsThreadUtils.h"
#include "js/Value.h"

class nsIDOMDOMRequest;

namespace mozilla {
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReply;

class BluetoothReplyRunnable : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  BluetoothReplyRunnable(nsIDOMDOMRequest* aReq,
                         Promise* aPromise = nullptr,
                         const nsAString& aName = EmptyString());

  void SetReply(BluetoothReply* aReply);

  void SetError(const nsAString& aErrorString,
                const enum BluetoothStatus aErrorStatus = STATUS_FAIL)
  {
    mErrorString = aErrorString;
    mErrorStatus = aErrorStatus;
  }

  virtual void ReleaseMembers();

protected:
  virtual ~BluetoothReplyRunnable();

  virtual bool ParseSuccessfulReply(JS::MutableHandle<JS::Value> aValue) = 0;

  
  
  
  nsAutoPtr<BluetoothReply> mReply;

private:
  nsresult FireReplySuccess(JS::Handle<JS::Value> aVal);
  nsresult FireErrorString();

  







  nsCOMPtr<nsIDOMDOMRequest> mDOMRequest;
  nsRefPtr<Promise> mPromise;

  BluetoothStatus mErrorStatus;
  nsString mErrorString;
  nsString mName; 
};

class BluetoothVoidReplyRunnable : public BluetoothReplyRunnable
{
public:
  BluetoothVoidReplyRunnable(nsIDOMDOMRequest* aReq,
                             Promise* aPromise = nullptr,
                             const nsAString& aName = EmptyString());
 ~BluetoothVoidReplyRunnable();

protected:
  virtual bool
  ParseSuccessfulReply(JS::MutableHandle<JS::Value> aValue) override
  {
    aValue.setUndefined();
    return true;
  }
};

END_BLUETOOTH_NAMESPACE

#endif
