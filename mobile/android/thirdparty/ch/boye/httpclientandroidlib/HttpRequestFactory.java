


























package ch.boye.httpclientandroidlib;






public interface HttpRequestFactory {

    HttpRequest newHttpRequest(RequestLine requestline)
        throws MethodNotSupportedException;

    HttpRequest newHttpRequest(String method, String uri)
            throws MethodNotSupportedException;

}
