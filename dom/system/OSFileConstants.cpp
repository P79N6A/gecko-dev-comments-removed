



#include "fcntl.h"
#include "errno.h"

#if defined(XP_UNIX)
#include "unistd.h"
#endif 

#if defined(XP_MACOSX)
#include "copyfile.h"
#endif 

#if defined(XP_WIN)
#include <windows.h>
#endif 

#include "jsapi.h"
#include "jsfriendapi.h"
#include "BindingUtils.h"



#include "nsIXULRuntime.h"
#include "nsXPCOMCIDInternal.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

#include "OSFileConstants.h"






namespace mozilla {









#define INT_CONSTANT(name)      \
  { #name, INT_TO_JSVAL(name) }




#define PROP_END { NULL, JSVAL_VOID }



#if !defined(S_IRGRP)
#define S_IXOTH 0001
#define S_IWOTH 0002
#define S_IROTH 0004
#define S_IRWXO 0007
#define S_IXGRP 0010
#define S_IWGRP 0020
#define S_IRGRP 0040
#define S_IRWXG 0070
#define S_IXUSR 0100
#define S_IWUSR 0200
#define S_IRUSR 0400
#define S_IRWXU 0700
#endif 









static dom::ConstantSpec gLibcProperties[] =
{
  
  INT_CONSTANT(O_APPEND),
  INT_CONSTANT(O_CREAT),
#if defined(O_DIRECTORY)
  INT_CONSTANT(O_DIRECTORY),
#endif 
#if defined(O_EVTONLY)
  INT_CONSTANT(O_EVTONLY),
#endif 
  INT_CONSTANT(O_EXCL),
#if defined(O_EXLOCK)
  INT_CONSTANT(O_EXLOCK),
#endif 
#if defined(O_LARGEFILE)
  INT_CONSTANT(O_LARGEFILE),
#endif 
#if defined(O_NOFOLLOW)
  INT_CONSTANT(O_NOFOLLOW),
#endif 
#if defined(O_NONBLOCK)
  INT_CONSTANT(O_NONBLOCK),
#endif 
  INT_CONSTANT(O_RDONLY),
  INT_CONSTANT(O_RDWR),
#if defined(O_RSYNC)
  INT_CONSTANT(O_RSYNC),
#endif 
#if defined(O_SHLOCK)
  INT_CONSTANT(O_SHLOCK),
#endif 
#if defined(O_SYMLINK)
  INT_CONSTANT(O_SYMLINK),
#endif 
#if defined(O_SYNC)
  INT_CONSTANT(O_SYNC),
#endif 
  INT_CONSTANT(O_TRUNC),
  INT_CONSTANT(O_WRONLY),

#if defined(AT_EACCESS)
  INT_CONSTANT(AT_EACCESS),
#endif 
#if defined(AT_FDCWD)
  INT_CONSTANT(AT_FDCWD),
#endif 
#if defined(AT_SYMLINK_NOFOLLOW)
  INT_CONSTANT(AT_SYMLINK_NOFOLLOW),
#endif 

  
#if defined(F_OK)
  INT_CONSTANT(F_OK),
  INT_CONSTANT(R_OK),
  INT_CONSTANT(W_OK),
  INT_CONSTANT(X_OK),
#endif 

  
  INT_CONSTANT(S_IRGRP),
  INT_CONSTANT(S_IROTH),
  INT_CONSTANT(S_IRUSR),
  INT_CONSTANT(S_IRWXG),
  INT_CONSTANT(S_IRWXO),
  INT_CONSTANT(S_IRWXU),
  INT_CONSTANT(S_IWGRP),
  INT_CONSTANT(S_IWOTH),
  INT_CONSTANT(S_IWUSR),
  INT_CONSTANT(S_IXOTH),
  INT_CONSTANT(S_IXGRP),
  INT_CONSTANT(S_IXUSR),

  
  INT_CONSTANT(SEEK_CUR),
  INT_CONSTANT(SEEK_END),
  INT_CONSTANT(SEEK_SET),

  
#if defined(COPYFILE_DATA)
  INT_CONSTANT(COPYFILE_DATA),
  INT_CONSTANT(COPYFILE_EXCL),
  INT_CONSTANT(COPYFILE_XATTR),
  INT_CONSTANT(COPYFILE_STAT),
  INT_CONSTANT(COPYFILE_ACL),
#endif 

  
  INT_CONSTANT(EACCES),
  INT_CONSTANT(EAGAIN),
  INT_CONSTANT(EBADF),
  INT_CONSTANT(EEXIST),
  INT_CONSTANT(EFAULT),
  INT_CONSTANT(EFBIG),
  INT_CONSTANT(EINVAL),
  INT_CONSTANT(EIO),
  INT_CONSTANT(EISDIR),
#if defined(ELOOP) 
  INT_CONSTANT(ELOOP),
#endif 
  INT_CONSTANT(EMFILE),
  INT_CONSTANT(ENAMETOOLONG),
  INT_CONSTANT(ENFILE),
  INT_CONSTANT(ENOENT),
  INT_CONSTANT(ENOMEM),
  INT_CONSTANT(ENOSPC),
  INT_CONSTANT(ENOTDIR),
  INT_CONSTANT(ENXIO),
#if defined(EOPNOTSUPP) 
  INT_CONSTANT(EOPNOTSUPP),
#endif 
#if defined(EOVERFLOW) 
  INT_CONSTANT(EOVERFLOW),
#endif 
  INT_CONSTANT(EPERM),
  INT_CONSTANT(ERANGE),
#if defined(ETIMEDOUT) 
  INT_CONSTANT(ETIMEDOUT),
#endif 
#if defined(EWOULDBLOCK) 
  INT_CONSTANT(EWOULDBLOCK),
#endif 
  INT_CONSTANT(EXDEV),

  PROP_END
};


#if defined(XP_WIN)








static dom::ConstantSpec gWinProperties[] =
{
  
  INT_CONSTANT(FORMAT_MESSAGE_FROM_SYSTEM),
  INT_CONSTANT(FORMAT_MESSAGE_IGNORE_INSERTS),

  
  INT_CONSTANT(GENERIC_ALL),
  INT_CONSTANT(GENERIC_EXECUTE),
  INT_CONSTANT(GENERIC_READ),
  INT_CONSTANT(GENERIC_WRITE),

  
  INT_CONSTANT(FILE_SHARE_DELETE),
  INT_CONSTANT(FILE_SHARE_READ),
  INT_CONSTANT(FILE_SHARE_WRITE),

  
  INT_CONSTANT(CREATE_ALWAYS),
  INT_CONSTANT(CREATE_NEW),
  INT_CONSTANT(OPEN_ALWAYS),
  INT_CONSTANT(OPEN_EXISTING),
  INT_CONSTANT(TRUNCATE_EXISTING),

  
  INT_CONSTANT(FILE_ATTRIBUTE_ARCHIVE),
  INT_CONSTANT(FILE_ATTRIBUTE_DIRECTORY),
  INT_CONSTANT(FILE_ATTRIBUTE_NORMAL),
  INT_CONSTANT(FILE_ATTRIBUTE_READONLY),
  INT_CONSTANT(FILE_ATTRIBUTE_TEMPORARY),

  
  { "INVALID_HANDLE_VALUE", INT_TO_JSVAL(INT_PTR(INVALID_HANDLE_VALUE)) },


  
  INT_CONSTANT(FILE_FLAG_DELETE_ON_CLOSE),

  
  INT_CONSTANT(FILE_BEGIN),
  INT_CONSTANT(FILE_CURRENT),
  INT_CONSTANT(FILE_END),

  
  INT_CONSTANT(INVALID_SET_FILE_POINTER),

  
  INT_CONSTANT(MOVEFILE_COPY_ALLOWED),
  INT_CONSTANT(MOVEFILE_REPLACE_EXISTING),

  
  INT_CONSTANT(ERROR_FILE_EXISTS),
  INT_CONSTANT(ERROR_FILE_NOT_FOUND),
  INT_CONSTANT(ERROR_ACCESS_DENIED),

  PROP_END
};
#endif 








JSObject *GetOrCreateObjectProperty(JSContext *cx, JSObject *aObject,
                                    const char *aProperty)
{
  JS::Value val;
  if (!JS_GetProperty(cx, aObject, aProperty, &val)) {
    return NULL;
  }
  if (!val.isUndefined()) {
    if (val.isObject()) {
      return &val.toObject();
    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
      JSMSG_UNEXPECTED_TYPE, aProperty, "not an object");
    return NULL;
  }
  return JS_DefineObject(cx, aObject, aProperty, NULL, NULL, JSPROP_ENUMERATE);
}







bool DefineOSFileConstants(JSContext *cx, JSObject *global)
{
  JSObject *objOS;
  if (!(objOS = GetOrCreateObjectProperty(cx, global, "OS"))) {
    return false;
  }
  JSObject *objConstants;
  if (!(objConstants = GetOrCreateObjectProperty(cx, objOS, "Constants"))) {
    return false;
  }
  JSObject *objLibc;
  if (!(objLibc = GetOrCreateObjectProperty(cx, objConstants, "libc"))) {
    return false;
  }
  if (!dom::DefineConstants(cx, objLibc, gLibcProperties)) {
    return false;
  }
#if defined(XP_WIN)
  JSObject *objWin;
  if (!(objWin = GetOrCreateObjectProperty(cx, objConstants, "Win"))) {
    return false;
  }
  if (!dom::DefineConstants(cx, objWin, gWinProperties)) {
    return false;
  }
#endif 
  JSObject *objSys;
  if (!(objSys = GetOrCreateObjectProperty(cx, objConstants, "Sys"))) {
    return false;
  }

  nsCOMPtr<nsIXULRuntime> runtime = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (runtime) {
    nsCAutoString os;
    nsresult rv = runtime->GetOS(os);
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    JSString* strVersion = JS_NewStringCopyZ(cx, os.get());
    if (!strVersion) {
      return false;
    }

    jsval valVersion = STRING_TO_JSVAL(strVersion);
    if (!JS_SetProperty(cx, objSys, "Version", &valVersion)) {
      return false;
    }
  }

  return true;
}

} 

