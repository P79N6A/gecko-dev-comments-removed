



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


void
VolumeManager::Dump(const char* aLabel)
{
  if (!sVolumeManager) {
    LOG("%s: sVolumeManager == null", aLabel);
    return;
  }

  VolumeArray::size_type  numVolumes = NumVolumes();
  VolumeArray::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = GetVolume(volIndex);
    vol->Dump(aLabel);
  }
}


size_t
VolumeManager::NumVolumes()
{
  if (!sVolumeManager) {
    return 0;
  }
  return sVolumeManager->mVolumeArray.Length();
}


already_AddRefed<Volume>
VolumeManager::GetVolume(size_t aIndex)
{
  MOZ_ASSERT(aIndex < NumVolumes());
  RefPtr<Volume> vol = sVolumeManager->mVolumeArray[aIndex];
  return vol.forget();
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


already_AddRefed<Volume>
VolumeManager::FindVolumeByName(const nsCSubstring& aName)
{
  if (!sVolumeManager) {
    return nullptr;
  }
  VolumeArray::size_type  numVolumes = NumVolumes();
  VolumeArray::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = GetVolume(volIndex);
    if (vol->Name().Equals(aName)) {
      return vol.forget();
    }
  }
  return nullptr;
}


already_AddRefed<Volume>
VolumeManager::FindAddVolumeByName(const nsCSubstring& aName)
{
  RefPtr<Volume> vol = FindVolumeByName(aName);
  if (vol) {
    return vol.forget();
  }
  
  vol = new Volume(aName);
  sVolumeManager->mVolumeArray.AppendElement(vol);
  return vol.forget();
}


void VolumeManager::InitConfig()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  
  
  
  
  
  
  
  

  ScopedCloseFile fp;
  int n = 0;
  char line[255];
  const char *filename = "/system/etc/volume.cfg";
  if (!(fp = fopen(filename, "r"))) {
    LOG("Unable to open volume configuration file '%s' - ignoring", filename);
    return;
  }
  while(fgets(line, sizeof(line), fp)) {
    n++;

    if (line[0] == '#')
      continue;

    nsCString commandline(line);
    nsCWhitespaceTokenizer tokenizer(commandline);
    if (!tokenizer.hasMoreTokens()) {
      
      continue;
    }

    nsCString command(tokenizer.nextToken());
    if (command.EqualsLiteral("create")) {
      if (!tokenizer.hasMoreTokens()) {
        ERR("No vol_name in %s line %d",  filename, n);
        continue;
      }
      nsCString volName(tokenizer.nextToken());
      if (!tokenizer.hasMoreTokens()) {
        ERR("No mount point for volume '%s'. %s line %d",
             volName.get(), filename, n);
        continue;
      }
      nsCString mountPoint(tokenizer.nextToken());
      RefPtr<Volume> vol = FindAddVolumeByName(volName);
      vol->SetFakeVolume(mountPoint);
      continue;
    }
    if (command.EqualsLiteral("configure")) {
      if (!tokenizer.hasMoreTokens()) {
        ERR("No vol_name in %s line %d", filename, n);
        continue;
      }
      nsCString volName(tokenizer.nextToken());
      if (!tokenizer.hasMoreTokens()) {
        ERR("No configuration name specified for volume '%s'. %s line %d",
             volName.get(), filename, n);
        continue;
      }
      nsCString configName(tokenizer.nextToken());
      if (!tokenizer.hasMoreTokens()) {
        ERR("No value for configuration name '%s'. %s line %d",
            configName.get(), filename, n);
        continue;
      }
      nsCString configValue(tokenizer.nextToken());
      RefPtr<Volume> vol = FindVolumeByName(volName);
      if (vol) {
        vol->SetConfig(configName, configValue);
      } else {
        ERR("Invalid volume name '%s'.", volName.get());
      }
      continue;
    }
    ERR("Unrecognized command: '%s'", command.get());
  }
}

void
VolumeManager::DefaultConfig()
{

  VolumeManager::VolumeArray::size_type numVolumes = VolumeManager::NumVolumes();
  if (numVolumes == 0) {
    return;
  }
  if (numVolumes == 1) {
    
    
    
    
    
    
    RefPtr<Volume> vol = VolumeManager::GetVolume(0);
    vol->SetIsRemovable(true);
    vol->SetIsHotSwappable(true);
    return;
  }
  VolumeManager::VolumeArray::index_type volIndex;
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = VolumeManager::GetVolume(volIndex);
    if (!vol->Name().EqualsLiteral("sdcard")) {
      vol->SetIsRemovable(true);
      vol->SetIsHotSwappable(true);
    }
  }
}

class VolumeListCallback : public VolumeResponseCallback
{
  virtual void ResponseReceived(const VolumeCommand* aCommand)
  {
    switch (ResponseCode()) {
      case ::ResponseCode::VolumeListResult: {
        
        
        
        
        
        
        nsCWhitespaceTokenizer tokenizer(ResponseStr());
        nsDependentCSubstring volName(tokenizer.nextToken());
        RefPtr<Volume> vol = VolumeManager::FindAddVolumeByName(volName);
        vol->HandleVoldResponse(ResponseCode(), tokenizer);
        break;
      }

      case ::ResponseCode::CommandOkay: {
        
        
        
        VolumeManager::DefaultConfig();
        VolumeManager::InitConfig();
        VolumeManager::Dump("READY");
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
  DBG("Wrote %d bytes (of %d)", bytesWritten, cmd->BytesRemaining());
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

  if (responseCode >= ::ResponseCode::UnsolicitedInformational) {
    
    
    HandleBroadcast(responseCode, responseLine);
  } else {
    
    if (mCommands.size() > 0) {
      VolumeCommand* cmd = mCommands.front();
      cmd->HandleResponse(responseCode, responseLine);
      if (responseCode >= ::ResponseCode::CommandOkay) {
        
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

  sVolumeManager = nullptr;
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
