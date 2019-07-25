


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;







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
        if (this.content == null) {
            throw new IllegalStateException("Content has not been provided");
        }
        return this.content;

    }

    




    public boolean isRepeatable() {
        return false;
    }

    





    public void setContentLength(long len) {
        this.length = len;
    }

    





    public void setContent(final InputStream instream) {
        this.content = instream;
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        if (outstream == null) {
            throw new IllegalArgumentException("Output stream may not be null");
        }
        InputStream instream = getContent();
        try {
            int l;
            byte[] tmp = new byte[2048];
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

    





    public void consumeContent() throws IOException {
        if (content != null) {
            content.close(); 
        }
    }

}
