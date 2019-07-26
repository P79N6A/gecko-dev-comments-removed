


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;
import java.util.Collection;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.client.params.ClientPNames;
import ch.boye.httpclientandroidlib.protocol.HttpContext;






@Immutable
public class RequestDefaultHeaders implements HttpRequestInterceptor {

    public RequestDefaultHeaders() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        if (request == null) {
            throw new IllegalArgumentException("HTTP request may not be null");
        }

        String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT")) {
            return;
        }

        
        @SuppressWarnings("unchecked")
        Collection<Header> defHeaders = (Collection<Header>) request.getParams().getParameter(
                ClientPNames.DEFAULT_HEADERS);

        if (defHeaders != null) {
            for (Header defHeader : defHeaders) {
                request.addHeader(defHeader);
            }
        }
    }

}
