


























package ch.boye.httpclientandroidlib.protocol;

















public interface HttpContext {

    
    public static final String RESERVED_PREFIX  = "http.";

    





    Object getAttribute(String id);

    





    void setAttribute(String id, Object obj);

    





    Object removeAttribute(String id);

}
