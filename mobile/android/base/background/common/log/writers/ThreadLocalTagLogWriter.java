



package org.mozilla.gecko.background.common.log.writers;






public class ThreadLocalTagLogWriter extends TagLogWriter {

  private final ThreadLocal<String> tag;

  public ThreadLocalTagLogWriter(ThreadLocal<String> tag, LogWriter inner) {
    super(inner);
    this.tag = tag;
  }

  @Override
  protected String getMainTag() {
    return this.tag.get();
  }
}
