


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
















@NotThreadSafe
public class HttpPut extends HttpEntityEnclosingRequestBase {

    public final static String METHOD_NAME = "PUT";

    public HttpPut() {
        super();
    }

    public HttpPut(final URI uri) {
        super();
        setURI(uri);
    }

    


    public HttpPut(final String uri) {
        super();
        setURI(URI.create(uri));
    }

    @Override
    public String getMethod() {
        return METHOD_NAME;
    }

}
