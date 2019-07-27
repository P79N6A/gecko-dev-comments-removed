



package org.mozilla.gecko.background.common.log.writers;




public class SimpleTagLogWriter extends TagLogWriter {
  final String tag;
  public SimpleTagLogWriter(String tag, LogWriter inner) {
    super(inner);
    this.tag = tag;
  }

  @Override
  protected String getMainTag() {
    return tag;
  }
}
