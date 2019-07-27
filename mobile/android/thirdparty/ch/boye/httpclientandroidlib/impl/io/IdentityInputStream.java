


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.util.Args;













@NotThreadSafe
public class IdentityInputStream extends InputStream {

    private final SessionInputBuffer in;

    private boolean closed = false;

    




    public IdentityInputStream(final SessionInputBuffer in) {
        super();
        this.in = Args.notNull(in, "Session input buffer");
    }

    @Override
    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            return ((BufferInfo) this.in).length();
        } else {
            return 0;
        }
    }

    @Override
    public void close() throws IOException {
        this.closed = true;
    }

    @Override
    public int read() throws IOException {
        if (this.closed) {
            return -1;
        } else {
            return this.in.read();
        }
    }

    @Override
    public int read(final byte[] b, final int off, final int len) throws IOException {
        if (this.closed) {
            return -1;
        } else {
            return this.in.read(b, off, len);
        }
    }

}
