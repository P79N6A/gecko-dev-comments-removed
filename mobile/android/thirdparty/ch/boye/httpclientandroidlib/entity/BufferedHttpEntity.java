


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;










@NotThreadSafe
public class BufferedHttpEntity extends HttpEntityWrapper {

    private final byte[] buffer;

    





    public BufferedHttpEntity(final HttpEntity entity) throws IOException {
        super(entity);
        if (!entity.isRepeatable() || entity.getContentLength() < 0) {
            this.buffer = EntityUtils.toByteArray(entity);
        } else {
            this.buffer = null;
        }
    }

    @Override
    public long getContentLength() {
        if (this.buffer != null) {
            return this.buffer.length;
        } else {
            return super.getContentLength();
        }
    }

    @Override
    public InputStream getContent() throws IOException {
        if (this.buffer != null) {
            return new ByteArrayInputStream(this.buffer);
        } else {
            return super.getContent();
        }
    }

    




    @Override
    public boolean isChunked() {
        return (buffer == null) && super.isChunked();
    }

    




    @Override
    public boolean isRepeatable() {
        return true;
    }


    @Override
    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        if (this.buffer != null) {
            outstream.write(this.buffer);
        } else {
            super.writeTo(outstream);
        }
    }


    
    @Override
    public boolean isStreaming() {
        return (buffer == null) && super.isStreaming();
    }

} 
