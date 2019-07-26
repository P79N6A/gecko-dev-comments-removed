



#include "VolumeManager.h"

#include "Volume.h"
#include "VolumeCommand.h"
#include "VolumeManagerLog.h"
#include "VolumeServiceTest.h"

#include "nsWhitespaceTokenizer.h"
#include "nsXULAppAPI.h"

#include "base/message_loop.h"
#include "mozilla/Scoped.h"
#include "mozilla/StaticPtr.h"

#include <android/log.h>
#include <cutils/sockets.h>
#include <fcntl.h>
#include <sys/socket.h>

namespace mozilla {
namespace system {

static StaticRefPtr<VolumeManager> sVolumeManager;

VolumeManager::STATE VolumeManager::mState = VolumeManager::UNINITIALIZED;
VolumeManager::StateObserverList VolumeManager::mStateObserverList;



VolumeManager::VolumeManager()
  : LineWatcher('\0', kRcvBufSize),
    mSocket(-1),
    mCommandPending(false)
{
  DBG("VolumeManager constructor called");
}

VolumeManager::~VolumeManager()
{
}


size_t
VolumeManager::NumVolumes()
{
  if (!sVolumeManager) {
    return 0;
  }
  return sVolumeManager->mVolumeArray.Length();
}


TemporaryRef<Volume>
VolumeManager::GetVolume(size_t aIndex)
{
  MOZ_ASSERT(aIndex < NumVolumes());
  return sVolumeManager->mVolumeArray[aIndex];
}


VolumeManager::STATE
VolumeManager::State()
{
  return mState;
}


const char *
VolumeManager::StateStr(VolumeManager::STATE aState)
{
  switch (aState) {
    case UNINITIALIZED: return "Uninitialized";
    case STARTING:      return "Starting";
    case VOLUMES_READY: return "Volumes Ready";
  }
  return "???";
}



void
VolumeManager::SetState(STATE aNewState)
{
  if (mState != aNewState) {
    LOG("changing state from '%s' to '%s'",
        StateStr(mState), StateStr(aNewState));
    mState = aNewState;
    mStateObserverList.Broadcast(StateChangedEvent());
  }
}


void
VolumeManager::RegisterStateObserver(StateObserver* aObserver)
{
  mStateObserverList.AddObserver(aObserver);
}


void VolumeManager::UnregisterStateObserver(StateObserver* aObserver)
{
  mStateObserverList.RemoveObserver(aObserver);
}


TemporaryRef<Volume>
VolumeManager::FindVolumeByName(const nsCSubstring& aName)
{
  if (!sVolumeManager) {
    return NULL;
  }
  VolumeArray::size_type  numVolumes = NumVolumes();
  VolumeArray::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = GetVolume(volIndex);
    if (vol->Name().Equals(aName)) {
      return vol;
    }
  }
  return NULL;
}


TemporaryRef<Volume>
VolumeManager::FindAddVolumeByName(const nsCSubstring& aName)
{
  RefPtr<Volume> vol = FindVolumeByName(aName);
  if (vol) {
    return vol;
  }
  
  vol = new Volume(aName);
  sVolumeManager->mVolumeArray.AppendElement(vol);
  return vol;
}

class VolumeListCallback : public VolumeResponseCallback
{
  virtual void ResponseReceived(const VolumeCommand* aCommand)
  {
    switch (ResponseCode()) {
      case ResponseCode::VolumeListResult: {
        
        
        
        
        
        
        nsCWhitespaceTokenizer tokenizer(ResponseStr());
        nsDependentCSubstring volName(tokenizer.nextToken());
        RefPtr<Volume> vol = VolumeManager::FindAddVolumeByName(volName);
        vol->HandleVoldResponse(ResponseCode(), tokenizer);
        break;
      }

      case ResponseCode::CommandOkay: {
        
        
        VolumeManager::SetState(VolumeManager::VOLUMES_READY);
        break;
      }
    }
  }
};

bool
VolumeManager::OpenSocket()
{
  SetState(STARTING);
  if ((mSocket.rwget() = socket_local_client("vold",
                                             ANDROID_SOCKET_NAMESPACE_RESERVED,
                                             SOCK_STREAM)) < 0) {
      ERR("Error connecting to vold: (%s) - will retry", strerror(errno));
      return false;
  }
  
  int flags = fcntl(mSocket.get(), F_GETFD);
  if (flags == -1) {
      return false;
  }
  flags |= FD_CLOEXEC;
  if (fcntl(mSocket.get(), F_SETFD, flags) == -1) {
    return false;
  }
  
  if (fcntl(mSocket.get(), F_SETFL, O_NONBLOCK) == -1) {
    return false;
  }
  if (!MessageLoopForIO::current()->
      WatchFileDescriptor(mSocket.get(),
                          true,
                          MessageLoopForIO::WATCH_READ,
                          &mReadWatcher,
                          this)) {
      return false;
  }

  LOG("Connected to vold");
  PostCommand(new VolumeListCommand(new VolumeListCallback));
  return true;
}


void
VolumeManager::PostCommand(VolumeCommand* aCommand)
{
  if (!sVolumeManager) {
    ERR("VolumeManager not initialized. Dropping command '%s'", aCommand->Data());
    return;
  }
  aCommand->SetPending(true);

  DBG("Sending command '%s'", aCommand->Data());
  
  
  sVolumeManager->mCommands.push(aCommand);
  if (!sVolumeManager->mCommandPending) {
    
    
    sVolumeManager->mCommandPending = true;
    sVolumeManager->WriteCommandData();
  }
}









void
VolumeManager::WriteCommandData()
{
  if (mCommands.size() == 0) {
    return;
  }

  VolumeCommand* cmd = mCommands.front();
  if (cmd->BytesRemaining() == 0) {
    
    return;
  }
  
  ssize_t bytesWritten = write(mSocket.get(), cmd->Data(), cmd->BytesRemaining());
  if (bytesWritten < 0) {
    ERR("Failed to write %d bytes to vold socket", cmd->BytesRemaining());
    Restart();
    return;
  }
  DBG("Wrote %ld bytes (of %d)", bytesWritten, cmd->BytesRemaining());
  cmd->ConsumeBytes(bytesWritten);
  if (cmd->BytesRemaining() == 0) {
    return;
  }
  
  
  if (!MessageLoopForIO::current()->
      WatchFileDescriptor(mSocket.get(),
                          false, 
                          MessageLoopForIO::WATCH_WRITE,
                          &mWriteWatcher,
                          this)) {
    ERR("Failed to setup write watcher for vold socket");
    Restart();
  }
}

void
VolumeManager::OnLineRead(int aFd, nsDependentCSubstring& aMessage)
{
  MOZ_ASSERT(aFd == mSocket.get());
  char* endPtr;
  int responseCode = strtol(aMessage.Data(), &endPtr, 10);
  if (*endPtr == ' ') {
    endPtr++;
  }

  
  nsDependentCString  responseLine(endPtr, aMessage.Length() - (endPtr - aMessage.Data()));
  DBG("Rcvd: %d '%s'", responseCode, responseLine.Data());

  if (responseCode >= ResponseCode::UnsolicitedInformational) {
    
    
    HandleBroadcast(responseCode, responseLine);
  } else {
    
    if (mCommands.size() > 0) {
      VolumeCommand* cmd = mCommands.front();
      cmd->HandleResponse(responseCode, responseLine);
      if (responseCode >= ResponseCode::CommandOkay) {
        
        mCommands.pop();
        mCommandPending = false;
        
        WriteCommandData();
      }
    } else {
      ERR("Response with no command");
    }
  }
}

void
VolumeManager::OnFileCanWriteWithoutBlocking(int aFd)
{
  MOZ_ASSERT(aFd == mSocket.get());
  WriteCommandData();
}

void
VolumeManager::HandleBroadcast(int aResponseCode, nsCString& aResponseLine)
{
  
  
  
  
  
  nsCWhitespaceTokenizer  tokenizer(aResponseLine);
  tokenizer.nextToken();  
  nsDependentCSubstring volName(tokenizer.nextToken());

  RefPtr<Volume> vol = FindVolumeByName(volName);
  if (!vol) {
    return;
  }
  vol->HandleVoldResponse(aResponseCode, tokenizer);
}

void
VolumeManager::Restart()
{
  mReadWatcher.StopWatchingFileDescriptor();
  mWriteWatcher.StopWatchingFileDescriptor();

  while (!mCommands.empty()) {
    mCommands.pop();
  }
  mCommandPending = false;
  mSocket.dispose();
  Start();
}


void
VolumeManager::Start()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  if (!sVolumeManager) {
    return;
  }
  SetState(STARTING);
  if (!sVolumeManager->OpenSocket()) {
    
    MessageLoopForIO::current()->
      PostDelayedTask(FROM_HERE,
                      NewRunnableFunction(VolumeManager::Start),
                      1000);
  }
}

void
VolumeManager::OnError()
{
  Restart();
}



static void
InitVolumeManagerIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(!sVolumeManager);

  sVolumeManager = new VolumeManager();
  VolumeManager::Start();

  InitVolumeServiceTestIOThread();
}

static void
ShutdownVolumeManagerIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  sVolumeManager = NULL;
}










void
InitVolumeManager()
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(InitVolumeManagerIOThread));
}

void
ShutdownVolumeManager()
{
  ShutdownVolumeServiceTest();

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ShutdownVolumeManagerIOThread));
}

} 
} 
