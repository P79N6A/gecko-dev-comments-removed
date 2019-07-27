





#ifndef mozilla_dom_bluetooth_bluetoothreplyrunnable_h__
#define mozilla_dom_bluetooth_bluetoothreplyrunnable_h__

#include "mozilla/Attributes.h"
#include "BluetoothCommon.h"
#include "nsThreadUtils.h"
#include "js/Value.h"

class nsIDOMDOMRequest;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReply;

class BluetoothReplyRunnable : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  BluetoothReplyRunnable(nsIDOMDOMRequest* aReq);

  void SetReply(BluetoothReply* aReply);

  void SetError(const nsAString& aError)
  {
    mErrorString = aError;
  }

  virtual void ReleaseMembers();

protected:
  virtual ~BluetoothReplyRunnable();

  virtual bool ParseSuccessfulReply(JS::MutableHandle<JS::Value> aValue) = 0;

  
  
  
  nsAutoPtr<BluetoothReply> mReply;

private:
  nsresult FireReply(JS::Handle<JS::Value> aVal);
  nsresult FireErrorString();

  nsCOMPtr<nsIDOMDOMRequest> mDOMRequest;
  nsString mErrorString;
};

class BluetoothVoidReplyRunnable : public BluetoothReplyRunnable
{
public:
  BluetoothVoidReplyRunnable(nsIDOMDOMRequest* aReq);
 ~BluetoothVoidReplyRunnable();

protected:
  virtual bool ParseSuccessfulReply(JS::MutableHandle<JS::Value> aValue) override
  {
    aValue.setUndefined();
    return true;
  }
};

END_BLUETOOTH_NAMESPACE

#endif
