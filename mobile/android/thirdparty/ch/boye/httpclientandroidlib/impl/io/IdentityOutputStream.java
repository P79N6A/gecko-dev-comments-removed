


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.util.Args;













@NotThreadSafe
public class IdentityOutputStream extends OutputStream {

    


    private final SessionOutputBuffer out;

    
    private boolean closed = false;

    public IdentityOutputStream(final SessionOutputBuffer out) {
        super();
        this.out = Args.notNull(out, "Session output buffer");
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
        this.out.write(b, off, len);
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
        this.out.write(b);
    }

}
