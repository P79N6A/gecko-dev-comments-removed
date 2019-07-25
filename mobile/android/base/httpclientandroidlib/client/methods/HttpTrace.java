


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;



















@NotThreadSafe
public class HttpTrace extends HttpRequestBase {

    public final static String METHOD_NAME = "TRACE";

    public HttpTrace() {
        super();
    }

    public HttpTrace(final URI uri) {
        super();
        setURI(uri);
    }

    


    public HttpTrace(final String uri) {
        super();
        setURI(URI.create(uri));
    }

    @Override
    public String getMethod() {
        return METHOD_NAME;
    }

}
