



































package org.mozilla.xpcom.internal;

import java.io.File;

import org.mozilla.xpcom.IJavaXPCOMUtils;


public class JavaXPCOMMethods implements IJavaXPCOMUtils {

  public static void registerJavaXPCOMMethods(File aLibXULDirectory) {
    
    String path = "";
    if (aLibXULDirectory != null) {
      path = aLibXULDirectory + File.separator;
    }

    String osName = System.getProperty("os.name").toLowerCase();
    if (osName.startsWith("os/2")) {
      System.load(path + System.mapLibraryName("jxpcmglu"));
    } else {
      System.load(path + System.mapLibraryName("javaxpcomglue"));
    }

    registerJavaXPCOMMethodsNative(aLibXULDirectory);
  }

  public static native void
      registerJavaXPCOMMethodsNative(File aLibXULDirectory);

  










  public static Class findClassInLoader(Object aObject, String aClassName) {
    try {
      if (aObject == null) {
        return Class.forName(aClassName);
      } else {
        return Class.forName(aClassName, true,
            aObject.getClass().getClassLoader());
      }
    } catch (ClassNotFoundException e) {
      return null;
    }
  }

  public native long wrapJavaObject(Object aJavaObject, String aIID);

  public native Object wrapXPCOMObject(long aXPCOMObject, String aIID);

}

