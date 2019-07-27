


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;
import ch.boye.httpclientandroidlib.protocol.HTTP;













@Deprecated
@NotThreadSafe 
public class EntityEnclosingRequestWrapper extends RequestWrapper
    implements HttpEntityEnclosingRequest {

    private HttpEntity entity;
    private boolean consumed;

    public EntityEnclosingRequestWrapper(final HttpEntityEnclosingRequest request)
        throws ProtocolException {
        super(request);
        setEntity(request.getEntity());
    }

    public HttpEntity getEntity() {
        return this.entity;
    }

    public void setEntity(final HttpEntity entity) {
        this.entity = entity != null ? new EntityWrapper(entity) : null;
        this.consumed = false;
    }

    public boolean expectContinue() {
        final Header expect = getFirstHeader(HTTP.EXPECT_DIRECTIVE);
        return expect != null && HTTP.EXPECT_CONTINUE.equalsIgnoreCase(expect.getValue());
    }

    @Override
    public boolean isRepeatable() {
        return this.entity == null || this.entity.isRepeatable() || !this.consumed;
    }

    class EntityWrapper extends HttpEntityWrapper {

        EntityWrapper(final HttpEntity entity) {
            super(entity);
        }

        @Override
        public void consumeContent() throws IOException {
            consumed = true;
            super.consumeContent();
        }

        @Override
        public InputStream getContent() throws IOException {
            consumed = true;
            return super.getContent();
        }

        @Override
        public void writeTo(final OutputStream outstream) throws IOException {
            consumed = true;
            super.writeTo(outstream);
        }

    }

}
