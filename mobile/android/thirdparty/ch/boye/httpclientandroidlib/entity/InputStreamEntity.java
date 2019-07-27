


























package ch.boye.httpclientandroidlib.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;







@NotThreadSafe
public class InputStreamEntity extends AbstractHttpEntity {

    private final InputStream content;
    private final long length;

    







    public InputStreamEntity(final InputStream instream) {
        this(instream, -1);
    }

    






    public InputStreamEntity(final InputStream instream, final long length) {
        this(instream, length, null);
    }

    








    public InputStreamEntity(final InputStream instream, final ContentType contentType) {
        this(instream, -1, contentType);
    }

    






    public InputStreamEntity(final InputStream instream, final long length, final ContentType contentType) {
        super();
        this.content = Args.notNull(instream, "Source input stream");
        this.length = length;
        if (contentType != null) {
            setContentType(contentType.toString());
        }
    }

    public boolean isRepeatable() {
        return false;
    }

    


    public long getContentLength() {
        return this.length;
    }

    public InputStream getContent() throws IOException {
        return this.content;
    }

    






    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final InputStream instream = this.content;
        try {
            final byte[] buffer = new byte[OUTPUT_BUFFER_SIZE];
            int l;
            if (this.length < 0) {
                
                while ((l = instream.read(buffer)) != -1) {
                    outstream.write(buffer, 0, l);
                }
            } else {
                
                long remaining = this.length;
                while (remaining > 0) {
                    l = instream.read(buffer, 0, (int)Math.min(OUTPUT_BUFFER_SIZE, remaining));
                    if (l == -1) {
                        break;
                    }
                    outstream.write(buffer, 0, l);
                    remaining -= l;
                }
            }
        } finally {
            instream.close();
        }
    }

    public boolean isStreaming() {
        return true;
    }

}
