


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;









@Immutable
public class RequestConnControl implements HttpRequestInterceptor {

    public RequestConnControl() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");

        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT")) {
            return;
        }

        if (!request.containsHeader(HTTP.CONN_DIRECTIVE)) {
            
            
            request.addHeader(HTTP.CONN_DIRECTIVE, HTTP.CONN_KEEP_ALIVE);
        }
    }

}
