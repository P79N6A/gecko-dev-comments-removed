




































package org.mozilla.xpcom.internal;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import org.mozilla.xpcom.XPCOMException;







public class XPCOMJavaProxy implements InvocationHandler {

  


  protected long nativeXPCOMPtr;

  




  public XPCOMJavaProxy(long aXPCOMInstance) {
    nativeXPCOMPtr = aXPCOMInstance;
  }

  






  protected static long getNativeXPCOMInstance(Object aProxy) {
    XPCOMJavaProxy proxy = (XPCOMJavaProxy) Proxy.getInvocationHandler(aProxy);
    return proxy.nativeXPCOMPtr;
  }

  







  protected static Object createProxy(Class aInterface, long aXPCOMInstance) {
    
    
    
    
    

    return Proxy.newProxyInstance(XPCOMJavaProxy.class.getClassLoader(),
            new Class[] { aInterface, XPCOMJavaProxyBase.class },
            new XPCOMJavaProxy(aXPCOMInstance));
  }

  










  public Object invoke(Object aProxy, Method aMethod, Object[] aParams)
          throws Throwable {
    String methodName = aMethod.getName();

    
    if (aMethod.getDeclaringClass() == Object.class)  {
      if (methodName.equals("hashCode"))  {
        return proxyHashCode(aProxy);
      }
      if (methodName.equals("equals")) {
        return proxyEquals(aProxy, aParams[0]);
      }
      if (methodName.equals("toString")) {
        return proxyToString(aProxy);
      }
      System.err.println("WARNING: Unhandled Object method [" +
                         methodName + "]");
      return null;
    }

    
    if (aMethod.getDeclaringClass() == XPCOMJavaProxyBase.class) {
      if (methodName.equals("finalize")) {
        finalizeProxy(aProxy);
      } else {
        System.err.println("WARNING: Unhandled XPCOMJavaProxyBase method [" +
                           methodName + "]");
      }
      return null;
    }

    
    return callXPCOMMethod(aProxy, methodName, aParams);
  }

  








  protected static Integer proxyHashCode(Object aProxy) {
    return new Integer(System.identityHashCode(aProxy));
  }

  










  protected static Boolean proxyEquals(Object aProxy, Object aOther) {
    
    if (aProxy == aOther) {
      return Boolean.TRUE;
    } else {
      
      
      if (isXPCOMJavaProxy(aOther) && isSameXPCOMObject(aProxy, aOther)) {
        return Boolean.TRUE;
      }
    }
    return Boolean.FALSE;
  }

  







  protected static boolean isXPCOMJavaProxy(Object aObject) {
    if (aObject != null && Proxy.isProxyClass(aObject.getClass())) {
      InvocationHandler h = Proxy.getInvocationHandler(aObject);
      if (h instanceof XPCOMJavaProxy) {
        return true;
      }
    }
    return false;
  }

  









  protected static native boolean isSameXPCOMObject(Object aProxy1,
          Object aProxy2);

  








  protected static String proxyToString(Object aProxy) {
    return aProxy.getClass().getInterfaces()[0].getName() + '@' +
           Integer.toHexString(aProxy.hashCode());
  }

  





  protected void finalizeProxy(Object aProxy) throws Throwable {
    finalizeProxyNative(aProxy);
    super.finalize();
  }

  protected static native void finalizeProxyNative(Object aProxy);

  











  protected static native Object callXPCOMMethod(Object aProxy,
          String aMethodName, Object[] aParams);

}

