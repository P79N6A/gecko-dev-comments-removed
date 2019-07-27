


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class ByteArrayEntity extends AbstractHttpEntity implements Cloneable {

    


    @Deprecated
    protected final byte[] content;
    private final byte[] b;
    private final int off, len;

    


    @SuppressWarnings("deprecation")
    public ByteArrayEntity(final byte[] b, final ContentType contentType) {
        super();
        Args.notNull(b, "Source byte array");
        this.content = b;
        this.b = b;
        this.off = 0;
        this.len = this.b.length;
        if (contentType != null) {
            setContentType(contentType.toString());
        }
    }

    


    @SuppressWarnings("deprecation")
    public ByteArrayEntity(final byte[] b, final int off, final int len, final ContentType contentType) {
        super();
        Args.notNull(b, "Source byte array");
        if ((off < 0) || (off > b.length) || (len < 0) ||
                ((off + len) < 0) || ((off + len) > b.length)) {
            throw new IndexOutOfBoundsException("off: " + off + " len: " + len + " b.length: " + b.length);
        }
        this.content = b;
        this.b = b;
        this.off = off;
        this.len = len;
        if (contentType != null) {
            setContentType(contentType.toString());
        }
    }

    public ByteArrayEntity(final byte[] b) {
        this(b, null);
    }

    public ByteArrayEntity(final byte[] b, final int off, final int len) {
        this(b, off, len, null);
    }

    public boolean isRepeatable() {
        return true;
    }

    public long getContentLength() {
        return this.len;
    }

    public InputStream getContent() {
        return new ByteArrayInputStream(this.b, this.off, this.len);
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        outstream.write(this.b, this.off, this.len);
        outstream.flush();
    }


    




    public boolean isStreaming() {
        return false;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

} 
