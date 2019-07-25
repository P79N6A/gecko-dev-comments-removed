





#ifndef mozilla_dom_bluetooth_bluetoothreplyrunnable_h__
#define mozilla_dom_bluetooth_bluetoothreplyrunnable_h__

#include "BluetoothCommon.h"
#include "nsThreadUtils.h"
#include "jsapi.h"

class nsIDOMDOMRequest;

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReply;

class BluetoothReplyRunnable : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  BluetoothReplyRunnable(nsIDOMDOMRequest* aReq) :
    mDOMRequest(aReq)
  {
  }

  void SetReply(BluetoothReply* aReply)
  {
    mReply = aReply;
  }

  void SetError(const nsAString& aError)
  {
    mErrorString = aError;
  }

  virtual void ReleaseMembers()
  {
    mDOMRequest = nullptr;
  }

protected:
  virtual ~BluetoothReplyRunnable()
  {
  }
  
  virtual bool ParseSuccessfulReply(jsval* aValue) = 0;

  
  
  
  nsAutoPtr<BluetoothReply> mReply;

private:
  nsresult FireReply(const jsval& aVal);
  nsresult FireErrorString();
  
  nsCOMPtr<nsIDOMDOMRequest> mDOMRequest;
  nsString mErrorString;
};

class BluetoothVoidReplyRunnable : public BluetoothReplyRunnable
{
public:
  BluetoothVoidReplyRunnable(nsIDOMDOMRequest* aReq) :
    BluetoothReplyRunnable(aReq)
  {
  }

  virtual void ReleaseMembers()
  {
    BluetoothReplyRunnable::ReleaseMembers();
  }
protected:
  virtual bool ParseSuccessfulReply(jsval* aValue)
  {
    *aValue = JSVAL_VOID;
    return true;
  }
};

END_BLUETOOTH_NAMESPACE

#endif
