


























package ch.boye.httpclientandroidlib.protocol;

import ch.boye.httpclientandroidlib.HttpConnection;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;







@NotThreadSafe
public class HttpCoreContext implements HttpContext {

    



    public static final String HTTP_CONNECTION  = "http.connection";

    



    public static final String HTTP_REQUEST     = "http.request";

    



    public static final String HTTP_RESPONSE    = "http.response";

    



    public static final String HTTP_TARGET_HOST = "http.target_host";

    




    public static final String HTTP_REQ_SENT    = "http.request_sent";

    public static HttpCoreContext create() {
        return new HttpCoreContext(new BasicHttpContext());
    }

    public static HttpCoreContext adapt(final HttpContext context) {
        Args.notNull(context, "HTTP context");
        if (context instanceof HttpCoreContext) {
            return (HttpCoreContext) context;
        } else {
            return new HttpCoreContext(context);
        }
    }

    private final HttpContext context;

    public HttpCoreContext(final HttpContext context) {
        super();
        this.context = context;
    }

    public HttpCoreContext() {
        super();
        this.context = new BasicHttpContext();
    }

    public Object getAttribute(final String id) {
        return context.getAttribute(id);
    }

    public void setAttribute(final String id, final Object obj) {
        context.setAttribute(id, obj);
    }

    public Object removeAttribute(final String id) {
        return context.removeAttribute(id);
    }

    public <T> T getAttribute(final String attribname, final Class<T> clazz) {
        Args.notNull(clazz, "Attribute class");
        final Object obj = getAttribute(attribname);
        if (obj == null) {
            return null;
        }
        return clazz.cast(obj);
    }

    public <T extends HttpConnection> T getConnection(final Class<T> clazz) {
        return getAttribute(HTTP_CONNECTION, clazz);
    }

    public HttpConnection getConnection() {
        return getAttribute(HTTP_CONNECTION, HttpConnection.class);
    }

    public HttpRequest getRequest() {
        return getAttribute(HTTP_REQUEST, HttpRequest.class);
    }

    public boolean isRequestSent() {
        final Boolean b = getAttribute(HTTP_REQ_SENT, Boolean.class);
        return b != null && b.booleanValue();
    }

    public HttpResponse getResponse() {
        return getAttribute(HTTP_RESPONSE, HttpResponse.class);
    }

    public void setTargetHost(final HttpHost host) {
        setAttribute(HTTP_TARGET_HOST, host);
    }

    public HttpHost getTargetHost() {
        return getAttribute(HTTP_TARGET_HOST, HttpHost.class);
    }

}
