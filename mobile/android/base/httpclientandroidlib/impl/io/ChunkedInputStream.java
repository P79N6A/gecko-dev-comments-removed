


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.MalformedChunkCodingException;
import ch.boye.httpclientandroidlib.TruncatedChunkException;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
import ch.boye.httpclientandroidlib.util.ExceptionUtils;

















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
        if (in == null) {
            throw new IllegalArgumentException("Session input buffer may not be null");
        }
        this.in = in;
        this.pos = 0;
        this.buffer = new CharArrayBuffer(16);
        this.state = CHUNK_LEN;
    }

    public int available() throws IOException {
        if (this.in instanceof BufferInfo) {
            int len = ((BufferInfo) this.in).length();
            return Math.min(len, this.chunkSize - this.pos);
        } else {
            return 0;
        }
    }

    











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
        int b = in.read();
        if (b != -1) {
            pos++;
            if (pos >= chunkSize) {
                state = CHUNK_CRLF;
            }
        }
        return b;
    }

    









    public int read (byte[] b, int off, int len) throws IOException {

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
        len = Math.min(len, chunkSize - pos);
        int bytesRead = in.read(b, off, len);
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

    






    public int read (byte[] b) throws IOException {
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
        int st = this.state;
        switch (st) {
        case CHUNK_CRLF:
            this.buffer.clear();
            int i = this.in.readLine(this.buffer);
            if (i == -1) {
                return 0;
            }
            if (!this.buffer.isEmpty()) {
                throw new MalformedChunkCodingException(
                    "Unexpected content at the end of chunk");
            }
            state = CHUNK_LEN;
            
        case CHUNK_LEN:
            this.buffer.clear();
            i = this.in.readLine(this.buffer);
            if (i == -1) {
                return 0;
            }
            int separator = this.buffer.indexOf(';');
            if (separator < 0) {
                separator = this.buffer.length();
            }
            try {
                return Integer.parseInt(this.buffer.substringTrimmed(0, separator), 16);
            } catch (NumberFormatException e) {
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
        } catch (HttpException e) {
            IOException ioe = new MalformedChunkCodingException("Invalid footer: "
                    + e.getMessage());
            ExceptionUtils.initCause(ioe, e);
            throw ioe;
        }
    }

    





    public void close() throws IOException {
        if (!closed) {
            try {
                if (!eof) {
                    
                    byte buffer[] = new byte[BUFFER_SIZE];
                    while (read(buffer) >= 0) {
                    }
                }
            } finally {
                eof = true;
                closed = true;
            }
        }
    }

    public Header[] getFooters() {
        return (Header[])this.footers.clone();
    }

}
