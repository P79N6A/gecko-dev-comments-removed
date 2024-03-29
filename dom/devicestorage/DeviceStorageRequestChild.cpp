






#include "DeviceStorageRequestChild.h"
#include "DeviceStorageFileDescriptor.h"
#include "nsDeviceStorage.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/ipc/BlobChild.h"

namespace mozilla {
namespace dom {
namespace devicestorage {

DeviceStorageRequestChild::DeviceStorageRequestChild()
  : mCallback(nullptr)
{
  MOZ_COUNT_CTOR(DeviceStorageRequestChild);
}

DeviceStorageRequestChild::DeviceStorageRequestChild(DOMRequest* aRequest,
                                                     DeviceStorageFile* aDSFile)
  : mRequest(aRequest)
  , mDSFile(aDSFile)
  , mCallback(nullptr)
{
  MOZ_ASSERT(aRequest);
  MOZ_ASSERT(aDSFile);
  MOZ_COUNT_CTOR(DeviceStorageRequestChild);
}

DeviceStorageRequestChild::DeviceStorageRequestChild(DOMRequest* aRequest,
                                                     DeviceStorageFile* aDSFile,
                                                     DeviceStorageFileDescriptor* aDSFileDescriptor)
  : mRequest(aRequest)
  , mDSFile(aDSFile)
  , mDSFileDescriptor(aDSFileDescriptor)
  , mCallback(nullptr)
{
  MOZ_ASSERT(aRequest);
  MOZ_ASSERT(aDSFile);
  MOZ_ASSERT(aDSFileDescriptor);
  MOZ_COUNT_CTOR(DeviceStorageRequestChild);
}

DeviceStorageRequestChild::~DeviceStorageRequestChild() {
  MOZ_COUNT_DTOR(DeviceStorageRequestChild);
}

bool
DeviceStorageRequestChild::
  Recv__delete__(const DeviceStorageResponseValue& aValue)
{
  if (mCallback) {
    mCallback->RequestComplete();
    mCallback = nullptr;
  }

  nsCOMPtr<nsPIDOMWindow> window = mRequest->GetOwner();
  if (!window) {
    return true;
  }

  switch (aValue.type()) {

    case DeviceStorageResponseValue::TErrorResponse:
    {
      ErrorResponse r = aValue;
      mRequest->FireError(r.error());
      break;
    }

    case DeviceStorageResponseValue::TSuccessResponse:
    {
      nsString fullPath;
      mDSFile->GetFullPath(fullPath);
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, fullPath, &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TFileDescriptorResponse:
    {
      FileDescriptorResponse r = aValue;

      nsString fullPath;
      mDSFile->GetFullPath(fullPath);
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, fullPath, &result);

      mDSFileDescriptor->mDSFile = mDSFile;
      mDSFileDescriptor->mFileDescriptor = r.fileDescriptor();
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TBlobResponse:
    {
      BlobResponse r = aValue;
      BlobChild* actor = static_cast<BlobChild*>(r.blobChild());
      nsRefPtr<BlobImpl> bloblImpl = actor->GetBlobImpl();
      nsRefPtr<Blob> blob = Blob::Create(mRequest->GetParentObject(),
                                         bloblImpl);

      AutoJSContext cx;

      JS::Rooted<JSObject*> obj(cx, blob->WrapObject(cx, nullptr));
      MOZ_ASSERT(obj);

      JS::Rooted<JS::Value> result(cx, JS::ObjectValue(*obj));
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TFreeSpaceStorageResponse:
    {
      FreeSpaceStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx, JS_NumberValue(double(r.freeBytes())));
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TUsedSpaceStorageResponse:
    {
      UsedSpaceStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx, JS_NumberValue(double(r.usedBytes())));
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TAvailableStorageResponse:
    {
      AvailableStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, r.mountState(), &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TStorageStatusResponse:
    {
      StorageStatusResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, r.storageStatus(), &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TFormatStorageResponse:
    {
      FormatStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, r.mountState(), &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TMountStorageResponse:
    {
      MountStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, r.storageStatus(), &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TUnmountStorageResponse:
    {
      UnmountStorageResponse r = aValue;
      AutoJSContext cx;
      JS::Rooted<JS::Value> result(cx);
      StringToJsval(window, r.storageStatus(), &result);
      mRequest->FireSuccess(result);
      break;
    }

    case DeviceStorageResponseValue::TEnumerationResponse:
    {
      EnumerationResponse r = aValue;
      nsDOMDeviceStorageCursor* cursor
        = static_cast<nsDOMDeviceStorageCursor*>(mRequest.get());

      uint32_t count = r.paths().Length();
      cursor->mFiles.SetCapacity(count);
      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<DeviceStorageFile> dsf
          = new DeviceStorageFile(r.type(), r.paths()[i].storageName(),
                                  r.rootdir(), r.paths()[i].name());
        cursor->mFiles.AppendElement(dsf.forget());
      }

      nsRefPtr<ContinueCursorEvent> event = new ContinueCursorEvent(cursor);
      event->Continue();
      break;
    }

    default:
    {
      NS_RUNTIMEABORT("not reached");
      break;
    }
  }
  return true;
}

void
DeviceStorageRequestChild::
  SetCallback(DeviceStorageRequestChildCallback *aCallback)
{
  mCallback = aCallback;
}

} 
} 
} 
