




































package org.mozilla.xpcom;







public class XPCOMException extends RuntimeException {

  


  public long errorcode;

  private static final long serialVersionUID = 198521829884000593L;

  



  public XPCOMException() {
    this(0x80004005L, "Unspecified internal XPCOM error");
  }

  





  public XPCOMException(String message) {
    this(0x80004005L, message);
  }

  





  public XPCOMException(long code) {
    this(code, "Internal XPCOM error");
  }

  





  public XPCOMException(long code, String message) {
    super(message + "  (0x" + Long.toHexString(code) + ")");
    this.errorcode = code;
  }

}

