


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;











@Immutable
public class ResponseContent implements HttpResponseInterceptor {

    private final boolean overwrite;

    




    public ResponseContent() {
        this(false);
    }

    










     public ResponseContent(final boolean overwrite) {
         super();
         this.overwrite = overwrite;
    }

    






    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");
        if (this.overwrite) {
            response.removeHeaders(HTTP.TRANSFER_ENCODING);
            response.removeHeaders(HTTP.CONTENT_LEN);
        } else {
            if (response.containsHeader(HTTP.TRANSFER_ENCODING)) {
                throw new ProtocolException("Transfer-encoding header already present");
            }
            if (response.containsHeader(HTTP.CONTENT_LEN)) {
                throw new ProtocolException("Content-Length header already present");
            }
        }
        final ProtocolVersion ver = response.getStatusLine().getProtocolVersion();
        final HttpEntity entity = response.getEntity();
        if (entity != null) {
            final long len = entity.getContentLength();
            if (entity.isChunked() && !ver.lessEquals(HttpVersion.HTTP_1_0)) {
                response.addHeader(HTTP.TRANSFER_ENCODING, HTTP.CHUNK_CODING);
            } else if (len >= 0) {
                response.addHeader(HTTP.CONTENT_LEN, Long.toString(entity.getContentLength()));
            }
            
            if (entity.getContentType() != null && !response.containsHeader(
                    HTTP.CONTENT_TYPE )) {
                response.addHeader(entity.getContentType());
            }
            
            if (entity.getContentEncoding() != null && !response.containsHeader(
                    HTTP.CONTENT_ENCODING)) {
                response.addHeader(entity.getContentEncoding());
            }
        } else {
            final int status = response.getStatusLine().getStatusCode();
            if (status != HttpStatus.SC_NO_CONTENT
                    && status != HttpStatus.SC_NOT_MODIFIED
                    && status != HttpStatus.SC_RESET_CONTENT) {
                response.addHeader(HTTP.CONTENT_LEN, "0");
            }
        }
    }

}
