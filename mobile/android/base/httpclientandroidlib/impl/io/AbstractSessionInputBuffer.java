


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;
import ch.boye.httpclientandroidlib.params.CoreConnectionPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.ByteArrayBuffer;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;




















public abstract class AbstractSessionInputBuffer implements SessionInputBuffer, BufferInfo {

    private InputStream instream;
    private byte[] buffer;
    private int bufferpos;
    private int bufferlen;

    private ByteArrayBuffer linebuffer = null;

    private String charset = HTTP.US_ASCII;
    private boolean ascii = true;
    private int maxLineLen = -1;
    private int minChunkLimit = 512;

    private HttpTransportMetricsImpl metrics;

    






    protected void init(final InputStream instream, int buffersize, final HttpParams params) {
        if (instream == null) {
            throw new IllegalArgumentException("Input stream may not be null");
        }
        if (buffersize <= 0) {
            throw new IllegalArgumentException("Buffer size may not be negative or zero");
        }
        if (params == null) {
            throw new IllegalArgumentException("HTTP parameters may not be null");
        }
        this.instream = instream;
        this.buffer = new byte[buffersize];
        this.bufferpos = 0;
        this.bufferlen = 0;
        this.linebuffer = new ByteArrayBuffer(buffersize);
        this.charset = HttpProtocolParams.getHttpElementCharset(params);
        this.ascii = this.charset.equalsIgnoreCase(HTTP.US_ASCII)
                     || this.charset.equalsIgnoreCase(HTTP.ASCII);
        this.maxLineLen = params.getIntParameter(CoreConnectionPNames.MAX_LINE_LENGTH, -1);
        this.minChunkLimit = params.getIntParameter(CoreConnectionPNames.MIN_CHUNK_LIMIT, 512);
        this.metrics = createTransportMetrics();
    }

    


    protected HttpTransportMetricsImpl createTransportMetrics() {
        return new HttpTransportMetricsImpl();
    }

    


    public int capacity() {
        return this.buffer.length;
    }

    


    public int length() {
        return this.bufferlen - this.bufferpos;
    }

    


    public int available() {
        return capacity() - length();
    }

    protected int fillBuffer() throws IOException {
        
        if (this.bufferpos > 0) {
            int len = this.bufferlen - this.bufferpos;
            if (len > 0) {
                System.arraycopy(this.buffer, this.bufferpos, this.buffer, 0, len);
            }
            this.bufferpos = 0;
            this.bufferlen = len;
        }
        int l;
        int off = this.bufferlen;
        int len = this.buffer.length - off;
        l = this.instream.read(this.buffer, off, len);
        if (l == -1) {
            return -1;
        } else {
            this.bufferlen = off + l;
            this.metrics.incrementBytesTransferred(l);
            return l;
        }
    }

    protected boolean hasBufferedData() {
        return this.bufferpos < this.bufferlen;
    }

    public int read() throws IOException {
        int noRead = 0;
        while (!hasBufferedData()) {
            noRead = fillBuffer();
            if (noRead == -1) {
                return -1;
            }
        }
        return this.buffer[this.bufferpos++] & 0xff;
    }

    public int read(final byte[] b, int off, int len) throws IOException {
        if (b == null) {
            return 0;
        }
        if (hasBufferedData()) {
            int chunk = Math.min(len, this.bufferlen - this.bufferpos);
            System.arraycopy(this.buffer, this.bufferpos, b, off, chunk);
            this.bufferpos += chunk;
            return chunk;
        }
        
        
        if (len > this.minChunkLimit) {
            int read = this.instream.read(b, off, len);
            if (read > 0) {
                this.metrics.incrementBytesTransferred(read);
            }
            return read;
        } else {
            
            while (!hasBufferedData()) {
                int noRead = fillBuffer();
                if (noRead == -1) {
                    return -1;
                }
            }
            int chunk = Math.min(len, this.bufferlen - this.bufferpos);
            System.arraycopy(this.buffer, this.bufferpos, b, off, chunk);
            this.bufferpos += chunk;
            return chunk;
        }
    }

    public int read(final byte[] b) throws IOException {
        if (b == null) {
            return 0;
        }
        return read(b, 0, b.length);
    }

    private int locateLF() {
        for (int i = this.bufferpos; i < this.bufferlen; i++) {
            if (this.buffer[i] == HTTP.LF) {
                return i;
            }
        }
        return -1;
    }

    














    public int readLine(final CharArrayBuffer charbuffer) throws IOException {
        if (charbuffer == null) {
            throw new IllegalArgumentException("Char array buffer may not be null");
        }
        int noRead = 0;
        boolean retry = true;
        while (retry) {
            
            int i = locateLF();
            if (i != -1) {
                
                if (this.linebuffer.isEmpty()) {
                    
                    return lineFromReadBuffer(charbuffer, i);
                }
                retry = false;
                int len = i + 1 - this.bufferpos;
                this.linebuffer.append(this.buffer, this.bufferpos, len);
                this.bufferpos = i + 1;
            } else {
                
                if (hasBufferedData()) {
                    int len = this.bufferlen - this.bufferpos;
                    this.linebuffer.append(this.buffer, this.bufferpos, len);
                    this.bufferpos = this.bufferlen;
                }
                noRead = fillBuffer();
                if (noRead == -1) {
                    retry = false;
                }
            }
            if (this.maxLineLen > 0 && this.linebuffer.length() >= this.maxLineLen) {
                throw new IOException("Maximum line length limit exceeded");
            }
        }
        if (noRead == -1 && this.linebuffer.isEmpty()) {
            
            return -1;
        }
        return lineFromLineBuffer(charbuffer);
    }

    












    private int lineFromLineBuffer(final CharArrayBuffer charbuffer)
            throws IOException {
        
        int l = this.linebuffer.length();
        if (l > 0) {
            if (this.linebuffer.byteAt(l - 1) == HTTP.LF) {
                l--;
                this.linebuffer.setLength(l);
            }
            
            if (l > 0) {
                if (this.linebuffer.byteAt(l - 1) == HTTP.CR) {
                    l--;
                    this.linebuffer.setLength(l);
                }
            }
        }
        l = this.linebuffer.length();
        if (this.ascii) {
            charbuffer.append(this.linebuffer, 0, l);
        } else {
            
            
            String s = new String(this.linebuffer.buffer(), 0, l, this.charset);
            l = s.length();
            charbuffer.append(s);
        }
        this.linebuffer.clear();
        return l;
    }

    private int lineFromReadBuffer(final CharArrayBuffer charbuffer, int pos)
            throws IOException {
        int off = this.bufferpos;
        int len;
        this.bufferpos = pos + 1;
        if (pos > 0 && this.buffer[pos - 1] == HTTP.CR) {
            
            pos--;
        }
        len = pos - off;
        if (this.ascii) {
            charbuffer.append(this.buffer, off, len);
        } else {
            
            
            String s = new String(this.buffer, off, len, this.charset);
            charbuffer.append(s);
            len = s.length();
        }
        return len;
    }

    public String readLine() throws IOException {
        CharArrayBuffer charbuffer = new CharArrayBuffer(64);
        int l = readLine(charbuffer);
        if (l != -1) {
            return charbuffer.toString();
        } else {
            return null;
        }
    }

    public HttpTransportMetrics getMetrics() {
        return this.metrics;
    }

}
