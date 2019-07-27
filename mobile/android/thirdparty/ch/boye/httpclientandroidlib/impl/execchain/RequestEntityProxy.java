

























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;






@NotThreadSafe
class RequestEntityProxy implements HttpEntity  {

    static void enhance(final HttpEntityEnclosingRequest request) {
        final HttpEntity entity = request.getEntity();
        if (entity != null && !entity.isRepeatable() && !isEnhanced(entity)) {
            request.setEntity(new RequestEntityProxy(entity));
        }
    }

    static boolean isEnhanced(final HttpEntity entity) {
        return entity instanceof RequestEntityProxy;
    }

    static boolean isRepeatable(final HttpRequest request) {
        if (request instanceof HttpEntityEnclosingRequest) {
            final HttpEntity entity = ((HttpEntityEnclosingRequest) request).getEntity();
            if (entity != null) {
                if (isEnhanced(entity)) {
                    final RequestEntityProxy proxy = (RequestEntityProxy) entity;
                    if (!proxy.isConsumed()) {
                        return true;
                    }
                }
                return entity.isRepeatable();
            }
        }
        return true;
    }

    private final HttpEntity original;
    private boolean consumed = false;

    RequestEntityProxy(final HttpEntity original) {
        super();
        this.original = original;
    }

    public HttpEntity getOriginal() {
        return original;
    }

    public boolean isConsumed() {
        return consumed;
    }

    public boolean isRepeatable() {
        return original.isRepeatable();
    }

    public boolean isChunked() {
        return original.isChunked();
    }

    public long getContentLength() {
        return original.getContentLength();
    }

    public Header getContentType() {
        return original.getContentType();
    }

    public Header getContentEncoding() {
        return original.getContentEncoding();
    }

    public InputStream getContent() throws IOException, IllegalStateException {
        return original.getContent();
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        consumed = true;
        original.writeTo(outstream);
    }

    public boolean isStreaming() {
        return original.isStreaming();
    }

    @Deprecated
    public void consumeContent() throws IOException {
        consumed = true;
        original.consumeContent();
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("RequestEntityProxy{");
        sb.append(original);
        sb.append('}');
        return sb.toString();
    }

}
