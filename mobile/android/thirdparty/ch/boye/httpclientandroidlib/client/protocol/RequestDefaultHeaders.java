


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;
import java.util.Collection;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.params.ClientPNames;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;






@SuppressWarnings("deprecation")
@Immutable
public class RequestDefaultHeaders implements HttpRequestInterceptor {

    private final Collection<? extends Header> defaultHeaders;

    


    public RequestDefaultHeaders(final Collection<? extends Header> defaultHeaders) {
        super();
        this.defaultHeaders = defaultHeaders;
    }

    public RequestDefaultHeaders() {
        this(null);
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");

        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT")) {
            return;
        }

        
        @SuppressWarnings("unchecked")
        Collection<? extends Header> defHeaders = (Collection<? extends Header>)
            request.getParams().getParameter(ClientPNames.DEFAULT_HEADERS);
        if (defHeaders == null) {
            defHeaders = this.defaultHeaders;
        }

        if (defHeaders != null) {
            for (final Header defHeader : defHeaders) {
                request.addHeader(defHeader);
            }
        }
    }

}
