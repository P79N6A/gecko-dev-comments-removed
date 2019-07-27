


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;










@NotThreadSafe
public class HttpEntityWrapper implements HttpEntity {

    
    protected HttpEntity wrappedEntity;

    


    public HttpEntityWrapper(final HttpEntity wrappedEntity) {
        super();
        this.wrappedEntity = Args.notNull(wrappedEntity, "Wrapped entity");
    } 

    public boolean isRepeatable() {
        return wrappedEntity.isRepeatable();
    }

    public boolean isChunked() {
        return wrappedEntity.isChunked();
    }

    public long getContentLength() {
        return wrappedEntity.getContentLength();
    }

    public Header getContentType() {
        return wrappedEntity.getContentType();
    }

    public Header getContentEncoding() {
        return wrappedEntity.getContentEncoding();
    }

    public InputStream getContent()
        throws IOException {
        return wrappedEntity.getContent();
    }

    public void writeTo(final OutputStream outstream)
        throws IOException {
        wrappedEntity.writeTo(outstream);
    }

    public boolean isStreaming() {
        return wrappedEntity.isStreaming();
    }

    



    @Deprecated
    public void consumeContent() throws IOException {
        wrappedEntity.consumeContent();
    }

}
