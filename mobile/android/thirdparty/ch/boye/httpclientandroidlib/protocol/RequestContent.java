


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;











@Immutable
public class RequestContent implements HttpRequestInterceptor {

    private final boolean overwrite;

    




    public RequestContent() {
        this(false);
    }

    










     public RequestContent(final boolean overwrite) {
         super();
         this.overwrite = overwrite;
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        if (request instanceof HttpEntityEnclosingRequest) {
            if (this.overwrite) {
                request.removeHeaders(HTTP.TRANSFER_ENCODING);
                request.removeHeaders(HTTP.CONTENT_LEN);
            } else {
                if (request.containsHeader(HTTP.TRANSFER_ENCODING)) {
                    throw new ProtocolException("Transfer-encoding header already present");
                }
                if (request.containsHeader(HTTP.CONTENT_LEN)) {
                    throw new ProtocolException("Content-Length header already present");
                }
            }
            final ProtocolVersion ver = request.getRequestLine().getProtocolVersion();
            final HttpEntity entity = ((HttpEntityEnclosingRequest)request).getEntity();
            if (entity == null) {
                request.addHeader(HTTP.CONTENT_LEN, "0");
                return;
            }
            
            if (entity.isChunked() || entity.getContentLength() < 0) {
                if (ver.lessEquals(HttpVersion.HTTP_1_0)) {
                    throw new ProtocolException(
                            "Chunked transfer encoding not allowed for " + ver);
                }
                request.addHeader(HTTP.TRANSFER_ENCODING, HTTP.CHUNK_CODING);
            } else {
                request.addHeader(HTTP.CONTENT_LEN, Long.toString(entity.getContentLength()));
            }
            
            if (entity.getContentType() != null && !request.containsHeader(
                    HTTP.CONTENT_TYPE )) {
                request.addHeader(entity.getContentType());
            }
            
            if (entity.getContentEncoding() != null && !request.containsHeader(
                    HTTP.CONTENT_ENCODING)) {
                request.addHeader(entity.getContentEncoding());
            }
        }
    }

}
