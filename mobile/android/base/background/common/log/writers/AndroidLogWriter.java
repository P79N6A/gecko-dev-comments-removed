



package org.mozilla.gecko.background.common.log.writers;

import android.util.Log;




public class AndroidLogWriter extends LogWriter {
  @Override
  public boolean shouldLogVerbose(String logTag) {
    return true;
  }

  @Override
  public void error(String tag, String message, Throwable error) {
    Log.e(tag, message, error);
  }

  @Override
  public void warn(String tag, String message, Throwable error) {
    Log.w(tag, message, error);
  }

  @Override
  public void info(String tag, String message, Throwable error) {
    Log.i(tag, message, error);
  }

  @Override
  public void debug(String tag, String message, Throwable error) {
    Log.d(tag, message, error);
  }

  @Override
  public void trace(String tag, String message, Throwable error) {
    Log.v(tag, message, error);
  }

  @Override
  public void close() {
  }
}
