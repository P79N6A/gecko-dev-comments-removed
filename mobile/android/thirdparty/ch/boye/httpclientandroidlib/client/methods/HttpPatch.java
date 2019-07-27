


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;


















@NotThreadSafe
public class HttpPatch extends HttpEntityEnclosingRequestBase {

    public final static String METHOD_NAME = "PATCH";

    public HttpPatch() {
        super();
    }

    public HttpPatch(final URI uri) {
        super();
        setURI(uri);
    }

    public HttpPatch(final String uri) {
        super();
        setURI(URI.create(uri));
    }

    @Override
    public String getMethod() {
        return METHOD_NAME;
    }

}
