


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;













public class IdentityInputStream extends InputStream {

    private final SessionInputBuffer in;

    private boolean closed = false;

    




    public IdentityInputStream(final SessionInputBuffer in) {
        super();
        if (in == null) {
            throw new IllegalArgumentException("Session input buffer may not be null");
        }
        this.in = in;
    }

    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            return ((BufferInfo) this.in).length();
        } else {
            return 0;
        }
    }

    public void close() throws IOException {
        this.closed = true;
    }

    public int read() throws IOException {
        if (this.closed) {
            return -1;
        } else {
            return this.in.read();
        }
    }

    public int read(final byte[] b, int off, int len) throws IOException {
        if (this.closed) {
            return -1;
        } else {
            return this.in.read(b, off, len);
        }
    }

}
