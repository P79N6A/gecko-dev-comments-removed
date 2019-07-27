


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;









@Immutable
public class ResponseConnControl implements HttpResponseInterceptor {

    public ResponseConnControl() {
        super();
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");

        final HttpCoreContext corecontext = HttpCoreContext.adapt(context);

        
        final int status = response.getStatusLine().getStatusCode();
        if (status == HttpStatus.SC_BAD_REQUEST ||
                status == HttpStatus.SC_REQUEST_TIMEOUT ||
                status == HttpStatus.SC_LENGTH_REQUIRED ||
                status == HttpStatus.SC_REQUEST_TOO_LONG ||
                status == HttpStatus.SC_REQUEST_URI_TOO_LONG ||
                status == HttpStatus.SC_SERVICE_UNAVAILABLE ||
                status == HttpStatus.SC_NOT_IMPLEMENTED) {
            response.setHeader(HTTP.CONN_DIRECTIVE, HTTP.CONN_CLOSE);
            return;
        }
        final Header explicit = response.getFirstHeader(HTTP.CONN_DIRECTIVE);
        if (explicit != null && HTTP.CONN_CLOSE.equalsIgnoreCase(explicit.getValue())) {
            
            return;
        }
        
        
        final HttpEntity entity = response.getEntity();
        if (entity != null) {
            final ProtocolVersion ver = response.getStatusLine().getProtocolVersion();
            if (entity.getContentLength() < 0 &&
                    (!entity.isChunked() || ver.lessEquals(HttpVersion.HTTP_1_0))) {
                response.setHeader(HTTP.CONN_DIRECTIVE, HTTP.CONN_CLOSE);
                return;
            }
        }
        
        final HttpRequest request = corecontext.getRequest();
        if (request != null) {
            final Header header = request.getFirstHeader(HTTP.CONN_DIRECTIVE);
            if (header != null) {
                response.setHeader(HTTP.CONN_DIRECTIVE, header.getValue());
            } else if (request.getProtocolVersion().lessEquals(HttpVersion.HTTP_1_0)) {
                response.setHeader(HTTP.CONN_DIRECTIVE, HTTP.CONN_CLOSE);
            }
        }
    }

}
