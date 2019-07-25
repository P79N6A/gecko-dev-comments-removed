



package org.mozilla.gecko.sync;

import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;

import org.mozilla.gecko.sync.log.writers.AndroidLevelCachingLogWriter;
import org.mozilla.gecko.sync.log.writers.AndroidLogWriter;
import org.mozilla.gecko.sync.log.writers.LogWriter;
import org.mozilla.gecko.sync.log.writers.SingleTagLogWriter;

import android.util.Log;




public class Logger {
  public static final String LOG_TAG = "Logger";

  public static final String GLOBAL_LOG_TAG = "FxSync";

  
  public static boolean LOG_PERSONAL_INFORMATION = false;

  





  protected final static Set<LogWriter> logWriters = new LinkedHashSet<LogWriter>(Logger.defaultLogWriters());

  


  protected final static Set<LogWriter> defaultLogWriters() {
    final Set<LogWriter> defaultLogWriters = new LinkedHashSet<LogWriter>();
    LogWriter log = new AndroidLogWriter();
    LogWriter cache = new AndroidLevelCachingLogWriter(log);
    final String processedPackage = GlobalConstants.BROWSER_INTENT_PACKAGE.replace("org.mozilla.", "");
    LogWriter single = new SingleTagLogWriter(processedPackage, new SingleTagLogWriter(GLOBAL_LOG_TAG, cache));
    defaultLogWriters.add(single);
    return defaultLogWriters;
  }

  public static synchronized void startLoggingTo(LogWriter logWriter) {
    logWriters.add(logWriter);
  }

  public static synchronized void stopLoggingTo(LogWriter logWriter) {
    try {
      logWriter.close();
    } catch (Exception e) {
      Log.e(LOG_TAG, "Got exception closing and removing LogWriter " + logWriter + ".", e);
    }
    logWriters.remove(logWriter);
  }

  public static synchronized void stopLoggingToAll() {
    for (LogWriter logWriter : logWriters) {
      try {
        logWriter.close();
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception closing and removing LogWriter " + logWriter + ".", e);
      }
    }
    logWriters.clear();
  }

  


  public static synchronized void resetLogging() {
    stopLoggingToAll();
    logWriters.addAll(Logger.defaultLogWriters());
  }

  
  public static synchronized boolean shouldLogVerbose(String logTag) {
    for (LogWriter logWriter : logWriters) {
      if (logWriter.shouldLogVerbose(logTag)) {
        return true;
      }
    }
    return false;
  }

  public static void error(String logTag, String message) {
    Logger.error(logTag, message, null);
  }

  public static void warn(String logTag, String message) {
    Logger.warn(logTag, message, null);
  }

  public static void info(String logTag, String message) {
    Logger.info(logTag, message, null);
  }

  public static void debug(String logTag, String message) {
    Logger.debug(logTag, message, null);
  }

  public static void trace(String logTag, String message) {
    Logger.trace(logTag, message, null);
  }

  public static void pii(String logTag, String message) {
    if (LOG_PERSONAL_INFORMATION) {
      Logger.debug(logTag, "$$PII$$: " + message);
    }
  }

  public static synchronized void error(String logTag, String message, Throwable error) {
    Iterator<LogWriter> it = logWriters.iterator();
    while (it.hasNext()) {
      LogWriter writer = it.next();
      try {
        writer.error(logTag, message, error);
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception logging; removing LogWriter " + writer + ".", e);
        it.remove();
      }
    }
  }

  public static synchronized void warn(String logTag, String message, Throwable error) {
    Iterator<LogWriter> it = logWriters.iterator();
    while (it.hasNext()) {
      LogWriter writer = it.next();
      try {
        writer.warn(logTag, message, error);
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception logging; removing LogWriter " + writer + ".", e);
        it.remove();
      }
    }
  }

  public static synchronized void info(String logTag, String message, Throwable error) {
    Iterator<LogWriter> it = logWriters.iterator();
    while (it.hasNext()) {
      LogWriter writer = it.next();
      try {
        writer.info(logTag, message, error);
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception logging; removing LogWriter " + writer + ".", e);
        it.remove();
      }
    }
  }

  public static synchronized void debug(String logTag, String message, Throwable error) {
    Iterator<LogWriter> it = logWriters.iterator();
    while (it.hasNext()) {
      LogWriter writer = it.next();
      try {
        writer.debug(logTag, message, error);
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception logging; removing LogWriter " + writer + ".", e);
        it.remove();
      }
    }
  }

  public static synchronized void trace(String logTag, String message, Throwable error) {
    Iterator<LogWriter> it = logWriters.iterator();
    while (it.hasNext()) {
      LogWriter writer = it.next();
      try {
        writer.trace(logTag, message, error);
      } catch (Exception e) {
        Log.e(LOG_TAG, "Got exception logging; removing LogWriter " + writer + ".", e);
        it.remove();
      }
    }
  }
}
