

























package ch.boye.httpclientandroidlib.impl.auth;

import java.io.IOException;
import java.io.OutputStream;
import java.security.MessageDigest;

class HttpEntityDigester extends OutputStream {

    private final MessageDigest digester;
    private boolean closed;
    private byte[] digest;

    HttpEntityDigester(final MessageDigest digester) {
        super();
        this.digester = digester;
        this.digester.reset();
    }

    @Override
    public void write(final int b) throws IOException {
        if (this.closed) {
            throw new IOException("Stream has been already closed");
        }
        this.digester.update((byte) b);
    }

    @Override
    public void write(final byte[] b, final int off, final int len) throws IOException {
        if (this.closed) {
            throw new IOException("Stream has been already closed");
        }
        this.digester.update(b, off, len);
    }

    @Override
    public void close() throws IOException {
        if (this.closed) {
            return;
        }
        this.closed = true;
        this.digest = this.digester.digest();
        super.close();
    }

    public byte[] getDigest() {
        return this.digest;
    }

}
