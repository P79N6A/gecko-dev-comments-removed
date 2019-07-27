


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.ConnectionClosedException;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.util.Args;

















@NotThreadSafe
public class ContentLengthInputStream extends InputStream {

    private static final int BUFFER_SIZE = 2048;
    



    private final long contentLength;

    
    private long pos = 0;

    
    private boolean closed = false;

    


    private SessionInputBuffer in = null;

    







    public ContentLengthInputStream(final SessionInputBuffer in, final long contentLength) {
        super();
        this.in = Args.notNull(in, "Session input buffer");
        this.contentLength = Args.notNegative(contentLength, "Content length");
    }

    






    @Override
    public void close() throws IOException {
        if (!closed) {
            try {
                if (pos < contentLength) {
                    final byte buffer[] = new byte[BUFFER_SIZE];
                    while (read(buffer) >= 0) {
                    }
                }
            } finally {
                
                
                closed = true;
            }
        }
    }

    @Override
    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            final int len = ((BufferInfo) this.in).length();
            return Math.min(len, (int) (this.contentLength - this.pos));
        } else {
            return 0;
        }
    }

    





    @Override
    public int read() throws IOException {
        if (closed) {
            throw new IOException("Attempted read from closed stream.");
        }

        if (pos >= contentLength) {
            return -1;
        }
        final int b = this.in.read();
        if (b == -1) {
            if (pos < contentLength) {
                throw new ConnectionClosedException(
                        "Premature end of Content-Length delimited message body (expected: "
                        + contentLength + "; received: " + pos);
            }
        } else {
            pos++;
        }
        return b;
    }

    











    @Override
    public int read (final byte[] b, final int off, final int len) throws java.io.IOException {
        if (closed) {
            throw new IOException("Attempted read from closed stream.");
        }

        if (pos >= contentLength) {
            return -1;
        }

        int chunk = len;
        if (pos + len > contentLength) {
            chunk = (int) (contentLength - pos);
        }
        final int count = this.in.read(b, off, chunk);
        if (count == -1 && pos < contentLength) {
            throw new ConnectionClosedException(
                    "Premature end of Content-Length delimited message body (expected: "
                    + contentLength + "; received: " + pos);
        }
        if (count > 0) {
            pos += count;
        }
        return count;
    }


    






    @Override
    public int read(final byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    







    @Override
    public long skip(final long n) throws IOException {
        if (n <= 0) {
            return 0;
        }
        final byte[] buffer = new byte[BUFFER_SIZE];
        
        
        long remaining = Math.min(n, this.contentLength - this.pos);
        
        long count = 0;
        while (remaining > 0) {
            final int l = read(buffer, 0, (int)Math.min(BUFFER_SIZE, remaining));
            if (l == -1) {
                break;
            }
            count += l;
            remaining -= l;
        }
        return count;
    }
}
