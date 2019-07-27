


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;













@NotThreadSafe
public class ChunkedOutputStream extends OutputStream {

    
    private final SessionOutputBuffer out;

    private final byte[] cache;

    private int cachePosition = 0;

    private boolean wroteLastChunk = false;

    
    private boolean closed = false;

    








    @Deprecated
    public ChunkedOutputStream(final SessionOutputBuffer out, final int bufferSize)
            throws IOException {
        this(bufferSize, out);
    }

    








    @Deprecated
    public ChunkedOutputStream(final SessionOutputBuffer out)
            throws IOException {
        this(2048, out);
    }

    





    public ChunkedOutputStream(final int bufferSize, final SessionOutputBuffer out) {
        super();
        this.cache = new byte[bufferSize];
        this.out = out;
    }

    


    protected void flushCache() throws IOException {
        if (this.cachePosition > 0) {
            this.out.writeLine(Integer.toHexString(this.cachePosition));
            this.out.write(this.cache, 0, this.cachePosition);
            this.out.writeLine("");
            this.cachePosition = 0;
        }
    }

    



    protected void flushCacheWithAppend(final byte bufferToAppend[], final int off, final int len) throws IOException {
        this.out.writeLine(Integer.toHexString(this.cachePosition + len));
        this.out.write(this.cache, 0, this.cachePosition);
        this.out.write(bufferToAppend, off, len);
        this.out.writeLine("");
        this.cachePosition = 0;
    }

    protected void writeClosingChunk() throws IOException {
        
        this.out.writeLine("0");
        this.out.writeLine("");
    }

    
    




    public void finish() throws IOException {
        if (!this.wroteLastChunk) {
            flushCache();
            writeClosingChunk();
            this.wroteLastChunk = true;
        }
    }

    
    @Override
    public void write(final int b) throws IOException {
        if (this.closed) {
            throw new IOException("Attempted write to closed stream.");
        }
        this.cache[this.cachePosition] = (byte) b;
        this.cachePosition++;
        if (this.cachePosition == this.cache.length) {
            flushCache();
        }
    }

    



    @Override
    public void write(final byte b[]) throws IOException {
        write(b, 0, b.length);
    }

    



    @Override
    public void write(final byte src[], final int off, final int len) throws IOException {
        if (this.closed) {
            throw new IOException("Attempted write to closed stream.");
        }
        if (len >= this.cache.length - this.cachePosition) {
            flushCacheWithAppend(src, off, len);
        } else {
            System.arraycopy(src, off, cache, this.cachePosition, len);
            this.cachePosition += len;
        }
    }

    


    @Override
    public void flush() throws IOException {
        flushCache();
        this.out.flush();
    }

    


    @Override
    public void close() throws IOException {
        if (!this.closed) {
            this.closed = true;
            finish();
            this.out.flush();
        }
    }
}
