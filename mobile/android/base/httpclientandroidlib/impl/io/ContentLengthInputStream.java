


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.ConnectionClosedException;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;

















public class ContentLengthInputStream extends InputStream {

    private static final int BUFFER_SIZE = 2048;
    



    private long contentLength;

    
    private long pos = 0;

    
    private boolean closed = false;

    


    private SessionInputBuffer in = null;

    







    public ContentLengthInputStream(final SessionInputBuffer in, long contentLength) {
        super();
        if (in == null) {
            throw new IllegalArgumentException("Input stream may not be null");
        }
        if (contentLength < 0) {
            throw new IllegalArgumentException("Content length may not be negative");
        }
        this.in = in;
        this.contentLength = contentLength;
    }

    






    public void close() throws IOException {
        if (!closed) {
            try {
                if (pos < contentLength) {
                    byte buffer[] = new byte[BUFFER_SIZE];
                    while (read(buffer) >= 0) {
                    }
                }
            } finally {
                
                
                closed = true;
            }
        }
    }

    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            int len = ((BufferInfo) this.in).length();
            return Math.min(len, (int) (this.contentLength - this.pos));
        } else {
            return 0;
        }
    }

    





    public int read() throws IOException {
        if (closed) {
            throw new IOException("Attempted read from closed stream.");
        }

        if (pos >= contentLength) {
            return -1;
        }
        int b = this.in.read();
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

    











    public int read (byte[] b, int off, int len) throws java.io.IOException {
        if (closed) {
            throw new IOException("Attempted read from closed stream.");
        }

        if (pos >= contentLength) {
            return -1;
        }

        if (pos + len > contentLength) {
            len = (int) (contentLength - pos);
        }
        int count = this.in.read(b, off, len);
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


    






    public int read(byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    







    public long skip(long n) throws IOException {
        if (n <= 0) {
            return 0;
        }
        byte[] buffer = new byte[BUFFER_SIZE];
        
        
        long remaining = Math.min(n, this.contentLength - this.pos);
        
        long count = 0;
        while (remaining > 0) {
            int l = read(buffer, 0, (int)Math.min(BUFFER_SIZE, remaining));
            if (l == -1) {
                break;
            }
            count += l;
            remaining -= l;
        }
        return count;
    }
}
