



package org.mozilla.gecko.background.common.log.writers;

import java.io.PrintWriter;




public class PrintLogWriter extends LogWriter {
  protected final PrintWriter pw;
  protected boolean closed = false;

  public static final String ERROR   = " :: E :: ";
  public static final String WARN    = " :: W :: ";
  public static final String INFO    = " :: I :: ";
  public static final String DEBUG   = " :: D :: ";
  public static final String VERBOSE = " :: V :: ";

  public PrintLogWriter(PrintWriter pw) {
    this.pw = pw;
  }

  protected void log(String tag, String message, Throwable error) {
    if (closed) {
      return;
    }

    pw.println(tag + message);
    if (error != null) {
      error.printStackTrace(pw);
    }
  }

  @Override
  public void error(String tag, String message, Throwable error) {
    log(tag, ERROR + message, error);
  }

  @Override
  public void warn(String tag, String message, Throwable error) {
    log(tag, WARN + message, error);
  }

  @Override
  public void info(String tag, String message, Throwable error) {
    log(tag, INFO + message, error);
  }

  @Override
  public void debug(String tag, String message, Throwable error) {
    log(tag, DEBUG + message, error);
  }

  @Override
  public void trace(String tag, String message, Throwable error) {
    log(tag, VERBOSE + message, error);
  }

  @Override
  public boolean shouldLogVerbose(String tag) {
    return true;
  }

  @Override
  public void close() {
    if (closed) {
      return;
    }
    if (pw != null) {
      pw.close();
    }
    closed = true;
  }
}
