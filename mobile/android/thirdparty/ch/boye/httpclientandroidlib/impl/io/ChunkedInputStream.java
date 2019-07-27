


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.MalformedChunkCodingException;
import ch.boye.httpclientandroidlib.TruncatedChunkException;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;

















@NotThreadSafe
public class ChunkedInputStream extends InputStream {

    private static final int CHUNK_LEN               = 1;
    private static final int CHUNK_DATA              = 2;
    private static final int CHUNK_CRLF              = 3;

    private static final int BUFFER_SIZE = 2048;

    
    private final SessionInputBuffer in;

    private final CharArrayBuffer buffer;

    private int state;

    
    private int chunkSize;

    
    private int pos;

    
    private boolean eof = false;

    
    private boolean closed = false;

    private Header[] footers = new Header[] {};

    




    public ChunkedInputStream(final SessionInputBuffer in) {
        super();
        this.in = Args.notNull(in, "Session input buffer");
        this.pos = 0;
        this.buffer = new CharArrayBuffer(16);
        this.state = CHUNK_LEN;
    }

    @Override
    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            final int len = ((BufferInfo) this.in).length();
            return Math.min(len, this.chunkSize - this.pos);
        } else {
            return 0;
        }
    }

    











    @Override
    public int read() throws IOException {
        if (this.closed) {
            throw new IOException("Attempted read from closed stream.");
        }
        if (this.eof) {
            return -1;
        }
        if (state != CHUNK_DATA) {
            nextChunk();
            if (this.eof) {
                return -1;
            }
        }
        final int b = in.read();
        if (b != -1) {
            pos++;
            if (pos >= chunkSize) {
                state = CHUNK_CRLF;
            }
        }
        return b;
    }

    









    @Override
    public int read (final byte[] b, final int off, final int len) throws IOException {

        if (closed) {
            throw new IOException("Attempted read from closed stream.");
        }

        if (eof) {
            return -1;
        }
        if (state != CHUNK_DATA) {
            nextChunk();
            if (eof) {
                return -1;
            }
        }
        final int bytesRead = in.read(b, off, Math.min(len, chunkSize - pos));
        if (bytesRead != -1) {
            pos += bytesRead;
            if (pos >= chunkSize) {
                state = CHUNK_CRLF;
            }
            return bytesRead;
        } else {
            eof = true;
            throw new TruncatedChunkException("Truncated chunk "
                    + "( expected size: " + chunkSize
                    + "; actual size: " + pos + ")");
        }
    }

    






    @Override
    public int read (final byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    



    private void nextChunk() throws IOException {
        chunkSize = getChunkSize();
        if (chunkSize < 0) {
            throw new MalformedChunkCodingException("Negative chunk size");
        }
        state = CHUNK_DATA;
        pos = 0;
        if (chunkSize == 0) {
            eof = true;
            parseTrailerHeaders();
        }
    }

    




    private int getChunkSize() throws IOException {
        final int st = this.state;
        switch (st) {
        case CHUNK_CRLF:
            this.buffer.clear();
            final int bytesRead1 = this.in.readLine(this.buffer);
            if (bytesRead1 == -1) {
                return 0;
            }
            if (!this.buffer.isEmpty()) {
                throw new MalformedChunkCodingException(
                    "Unexpected content at the end of chunk");
            }
            state = CHUNK_LEN;
            
        case CHUNK_LEN:
            this.buffer.clear();
            final int bytesRead2 = this.in.readLine(this.buffer);
            if (bytesRead2 == -1) {
                return 0;
            }
            int separator = this.buffer.indexOf(';');
            if (separator < 0) {
                separator = this.buffer.length();
            }
            try {
                return Integer.parseInt(this.buffer.substringTrimmed(0, separator), 16);
            } catch (final NumberFormatException e) {
                throw new MalformedChunkCodingException("Bad chunk header");
            }
        default:
            throw new IllegalStateException("Inconsistent codec state");
        }
    }

    



    private void parseTrailerHeaders() throws IOException {
        try {
            this.footers = AbstractMessageParser.parseHeaders
                (in, -1, -1, null);
        } catch (final HttpException ex) {
            final IOException ioe = new MalformedChunkCodingException("Invalid footer: "
                    + ex.getMessage());
            ioe.initCause(ex);
            throw ioe;
        }
    }

    





    @Override
    public void close() throws IOException {
        if (!closed) {
            try {
                if (!eof) {
                    
                    final byte buff[] = new byte[BUFFER_SIZE];
                    while (read(buff) >= 0) {
                    }
                }
            } finally {
                eof = true;
                closed = true;
            }
        }
    }

    public Header[] getFooters() {
        return this.footers.clone();
    }

}
