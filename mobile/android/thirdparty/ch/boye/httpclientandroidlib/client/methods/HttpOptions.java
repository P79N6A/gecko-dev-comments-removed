


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;
import java.util.HashSet;
import java.util.Set;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;


















@NotThreadSafe
public class HttpOptions extends HttpRequestBase {

    public final static String METHOD_NAME = "OPTIONS";

    public HttpOptions() {
        super();
    }

    public HttpOptions(final URI uri) {
        super();
        setURI(uri);
    }

    


    public HttpOptions(final String uri) {
        super();
        setURI(URI.create(uri));
    }

    @Override
    public String getMethod() {
        return METHOD_NAME;
    }

    public Set<String> getAllowedMethods(final HttpResponse response) {
        Args.notNull(response, "HTTP response");

        final HeaderIterator it = response.headerIterator("Allow");
        final Set<String> methods = new HashSet<String>();
        while (it.hasNext()) {
            final Header header = it.nextHeader();
            final HeaderElement[] elements = header.getElements();
            for (final HeaderElement element : elements) {
                methods.add(element.getName());
            }
        }
        return methods;
    }

}
