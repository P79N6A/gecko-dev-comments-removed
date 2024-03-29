



package org.mozilla.gecko.background.common.log.writers;

import android.util.Log;






public class LevelFilteringLogWriter extends LogWriter {
  protected final LogWriter inner;
  protected final int logLevel;

  public LevelFilteringLogWriter(int logLevel, LogWriter inner) {
    this.inner = inner;
    this.logLevel = logLevel;
  }

  @Override
  public void close() {
    inner.close();
  }

  @Override
  public void error(String tag, String message, Throwable error) {
    if (logLevel <= Log.ERROR) {
      inner.error(tag, message, error);
    }
  }

  @Override
  public void warn(String tag, String message, Throwable error) {
    if (logLevel <= Log.WARN) {
      inner.warn(tag, message, error);
    }
  }

  @Override
  public void info(String tag, String message, Throwable error) {
    if (logLevel <= Log.INFO) {
      inner.info(tag, message, error);
    }
  }

  @Override
  public void debug(String tag, String message, Throwable error) {
    if (logLevel <= Log.DEBUG) {
      inner.debug(tag, message, error);
    }
  }

  @Override
  public void trace(String tag, String message, Throwable error) {
    if (logLevel <= Log.VERBOSE) {
      inner.trace(tag, message, error);
    }
  }

  @Override
  public boolean shouldLogVerbose(String tag) {
    return logLevel <= Log.VERBOSE;
  }
}
