


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

















@NotThreadSafe
public class HttpGet extends HttpRequestBase {

    public final static String METHOD_NAME = "GET";

    public HttpGet() {
        super();
    }

    public HttpGet(final URI uri) {
        super();
        setURI(uri);
    }

    


    public HttpGet(final String uri) {
        super();
        setURI(URI.create(uri));
    }

    @Override
    public String getMethod() {
        return METHOD_NAME;
    }

}
