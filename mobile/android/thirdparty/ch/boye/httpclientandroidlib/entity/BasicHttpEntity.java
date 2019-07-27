


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;







@NotThreadSafe
public class BasicHttpEntity extends AbstractHttpEntity {

    private InputStream content;
    private long length;

    




    public BasicHttpEntity() {
        super();
        this.length = -1;
    }

    public long getContentLength() {
        return this.length;
    }

    








    public InputStream getContent() throws IllegalStateException {
        Asserts.check(this.content != null, "Content has not been provided");
        return this.content;
    }

    




    public boolean isRepeatable() {
        return false;
    }

    





    public void setContentLength(final long len) {
        this.length = len;
    }

    





    public void setContent(final InputStream instream) {
        this.content = instream;
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final InputStream instream = getContent();
        try {
            int l;
            final byte[] tmp = new byte[OUTPUT_BUFFER_SIZE];
            while ((l = instream.read(tmp)) != -1) {
                outstream.write(tmp, 0, l);
            }
        } finally {
            instream.close();
        }
    }

    public boolean isStreaming() {
        return this.content != null;
    }

}
