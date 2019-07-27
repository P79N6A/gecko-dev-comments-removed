


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.util.Args;














@NotThreadSafe
public class ContentLengthOutputStream extends OutputStream {

    


    private final SessionOutputBuffer out;

    



    private final long contentLength;

    
    private long total = 0;

    
    private boolean closed = false;

    









    public ContentLengthOutputStream(final SessionOutputBuffer out, final long contentLength) {
        super();
        this.out = Args.notNull(out, "Session output buffer");
        this.contentLength = Args.notNegative(contentLength, "Content length");
    }

    




    @Override
    public void close() throws IOException {
        if (!this.closed) {
            this.closed = true;
            this.out.flush();
        }
    }

    @Override
    public void flush() throws IOException {
        this.out.flush();
    }

    @Override
    public void write(final byte[] b, final int off, final int len) throws IOException {
        if (this.closed) {
            throw new IOException("Attempted write to closed stream.");
        }
        if (this.total < this.contentLength) {
            final long max = this.contentLength - this.total;
            int chunk = len;
            if (chunk > max) {
                chunk = (int) max;
            }
            this.out.write(b, off, chunk);
            this.total += chunk;
        }
    }

    @Override
    public void write(final byte[] b) throws IOException {
        write(b, 0, b.length);
    }

    @Override
    public void write(final int b) throws IOException {
        if (this.closed) {
            throw new IOException("Attempted write to closed stream.");
        }
        if (this.total < this.contentLength) {
            this.out.write(b);
            this.total++;
        }
    }

}
