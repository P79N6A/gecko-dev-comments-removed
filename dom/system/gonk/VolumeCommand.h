



#ifndef mozilla_system_volumecommand_h__
#define mozilla_system_volumecommand_h__

#include "nsString.h"
#include "nsISupportsImpl.h"
#include "mozilla/RefPtr.h"
#include <algorithm>
#include <vold/ResponseCode.h>

namespace mozilla {
namespace system {

class Volume;
class VolumeCommand;

















class VolumeResponseCallback
{
protected:
  virtual ~VolumeResponseCallback() {}

public:
  NS_INLINE_DECL_REFCOUNTING(VolumeResponseCallback)
  VolumeResponseCallback()
    : mResponseCode(0), mPending(false) {}

  bool Done() const
  {
    
    

    return (mResponseCode >= ResponseCode::CommandOkay)
        && (mResponseCode < ResponseCode::UnsolicitedInformational);
  }

  bool WasSuccessful() const
  {
    return mResponseCode == ResponseCode::CommandOkay;
  }

  bool              IsPending() const     { return mPending; }
  int               ResponseCode() const  { return mResponseCode; }
  const nsCString  &ResponseStr() const   { return mResponseStr; }

protected:
  virtual void ResponseReceived(const VolumeCommand* aCommand) = 0;

private:
  friend  class VolumeCommand;  

  void HandleResponse(const VolumeCommand* aCommand,
                      int aResponseCode,
                      nsACString& aResponseStr)
  {
    mResponseCode = aResponseCode;
#if ANDROID_VERSION >= 17
    
    
    mResponseStr = Substring(aResponseStr, 2);
#else
    mResponseStr = aResponseStr;
#endif
    if (mResponseCode >= ResponseCode::CommandOkay) {
      
      mPending = false;
    }
    ResponseReceived(aCommand);
  }

  void SetPending(bool aPending) { mPending = aPending; }

  int       mResponseCode;  
  nsCString mResponseStr;   
  bool      mPending;       
};
















class VolumeCommand
{
protected:
  virtual ~VolumeCommand() {}

public:
  NS_INLINE_DECL_REFCOUNTING(VolumeCommand)

  VolumeCommand(VolumeResponseCallback* aCallback)
    : mBytesConsumed(0),
      mCallback(aCallback)
  {
    SetCmd(NS_LITERAL_CSTRING(""));
  }

  VolumeCommand(const nsACString& aCommand, VolumeResponseCallback* aCallback)
    : mBytesConsumed(0),
      mCallback(aCallback)
  {
    SetCmd(aCommand);
  }

  void SetCmd(const nsACString& aCommand)
  {
    mCmd.Truncate();
#if ANDROID_VERSION >= 17
    
    
    mCmd = "0 ";
#endif
    mCmd.Append(aCommand);
    
    
    mCmd.Append('\0');
  }

  const char* CmdStr() const    { return mCmd.get(); }
  const char* Data() const      { return mCmd.Data() + mBytesConsumed; }
  size_t BytesConsumed() const  { return mBytesConsumed; }

  size_t BytesRemaining() const
  {
    return mCmd.Length() - std::min(mCmd.Length(), mBytesConsumed);
  }

  void ConsumeBytes(size_t aNumBytes)
  {
    mBytesConsumed += std::min(BytesRemaining(), aNumBytes);
  }

private:
  friend class VolumeManager;   

  void SetPending(bool aPending)
  {
    if (mCallback) {
      mCallback->SetPending(aPending);
    }
  }

  void HandleResponse(int aResponseCode, nsACString& aResponseStr)
  {
    if (mCallback) {
      mCallback->HandleResponse(this, aResponseCode, aResponseStr);
    }
  }

  nsCString mCmd;           
  size_t    mBytesConsumed; 

  
  RefPtr<VolumeResponseCallback>  mCallback;
};

class VolumeActionCommand : public VolumeCommand
{
public:
  VolumeActionCommand(Volume* aVolume, const char* aAction,
                      const char* aExtraArgs, VolumeResponseCallback* aCallback);

private:
  RefPtr<Volume>  mVolume;
};

class VolumeListCommand : public VolumeCommand
{
public:
  VolumeListCommand(VolumeResponseCallback* aCallback);
};

} 
} 

#endif  
