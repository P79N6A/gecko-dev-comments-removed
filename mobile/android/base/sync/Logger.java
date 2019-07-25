



package org.mozilla.gecko.sync;

import java.util.IdentityHashMap;
import java.util.Map;

import android.util.Log;










public class Logger {

  
  public static boolean LOG_PERSONAL_INFORMATION = false;

  
  public static boolean LOG_TO_STDOUT = false;

  
  
  
  private static Map<String, Boolean> isErrorLoggable   = new IdentityHashMap<String, Boolean>();
  private static Map<String, Boolean> isWarnLoggable    = new IdentityHashMap<String, Boolean>();
  private static Map<String, Boolean> isInfoLoggable    = new IdentityHashMap<String, Boolean>();
  private static Map<String, Boolean> isDebugLoggable   = new IdentityHashMap<String, Boolean>();
  private static Map<String, Boolean> isVerboseLoggable = new IdentityHashMap<String, Boolean>();

  


  public synchronized void refreshLogLevels() {
    isErrorLoggable   = new IdentityHashMap<String, Boolean>();
    isWarnLoggable    = new IdentityHashMap<String, Boolean>();
    isInfoLoggable    = new IdentityHashMap<String, Boolean>();
    isDebugLoggable   = new IdentityHashMap<String, Boolean>();
    isVerboseLoggable = new IdentityHashMap<String, Boolean>();    
  }

  private static boolean shouldLogError(String logTag) {
    Boolean out = isErrorLoggable.get(logTag);
    if (out != null) {
      return out.booleanValue();
    }
    out = Log.isLoggable(logTag, Log.ERROR);
    isErrorLoggable.put(logTag, out);
    return out;
  }

  private static boolean shouldLogWarn(String logTag) {
    Boolean out = isWarnLoggable.get(logTag);
    if (out != null) {
      return out.booleanValue();
    }
    out = Log.isLoggable(logTag, Log.WARN);
    isWarnLoggable.put(logTag, out);
    return out;
  }

  private static boolean shouldLogInfo(String logTag) {
    Boolean out = isInfoLoggable.get(logTag);
    if (out != null) {
      return out.booleanValue();
    }
    Log.d("XXX", "Calling out to isLoggable for INFO!");
    out = Log.isLoggable(logTag, Log.INFO);
    isInfoLoggable.put(logTag, out);
    return out;
  }

  private static boolean shouldLogDebug(String logTag) {
    Boolean out = isDebugLoggable.get(logTag);
    if (out != null) {
      return out.booleanValue();
    }
    Log.d("XXX", "Calling out to isLoggable for DEBUG!");
    out = Log.isLoggable(logTag, Log.DEBUG);
    isDebugLoggable.put(logTag, out);
    return out;
  }

  private static boolean shouldLogVerbose(String logTag) {
    Boolean out = isVerboseLoggable.get(logTag);
    if (out != null) {
      return out.booleanValue();
    }
    Log.d("XXX", "Calling out to isLoggable for VERBOSE!");
    out = Log.isLoggable(logTag, Log.VERBOSE);
    isVerboseLoggable.put(logTag, out);
    return out;
  }

  
  public static synchronized boolean logVerbose(String logTag) {
    return shouldLogVerbose(logTag);
  }

  private static void logToStdout(String... s) {
    if (LOG_TO_STDOUT) {
      for (String string : s) {
        System.out.print(string);
      }
      System.out.println("");
    }
  }

  public static void error(String logTag, String message) {
    Logger.error(logTag, message, null);
  }

  public static synchronized void error(String logTag, String message, Throwable error) {
    logToStdout(logTag, " :: ERROR: ", message);
    if (shouldLogError(logTag)) {
      Log.e(logTag, message, error);
    }
  }

  public static void warn(String logTag, String message) {
    Logger.warn(logTag, message, null);
  }

  public static synchronized void warn(String logTag, String message, Throwable error) {
    logToStdout(logTag, " :: WARN: ", message);
    if (shouldLogWarn(logTag)) {
      Log.w(logTag, message, error);
    }
  }

  public static synchronized void info(String logTag, String message) {
    logToStdout(logTag, " :: INFO: ", message);
    if (shouldLogInfo(logTag)) {
      Log.i(logTag, message);
    }
  }

  public static void debug(String logTag, String message) {
    Logger.debug(logTag, message, null);
  }

  public static synchronized void debug(String logTag, String message, Throwable error) {
    logToStdout(logTag, " :: DEBUG: ", message);
    if (shouldLogDebug(logTag)) {
      Log.d(logTag, message, error);
    }
  }

  public static synchronized void trace(String logTag, String message) {
    logToStdout(logTag, " :: TRACE: ", message);
    if (shouldLogVerbose(logTag)) {
      Log.v(logTag, message);
    }
  }

  public static void pii(String logTag, String message) {
    if (LOG_PERSONAL_INFORMATION) {
      Logger.debug(logTag, "$$PII$$: " + message);
    }
  }
}
