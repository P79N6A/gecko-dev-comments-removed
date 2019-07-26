



package org.mozilla.gecko.sync.log.writers;






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
