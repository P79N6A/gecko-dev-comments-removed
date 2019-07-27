


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CoderResult;
import java.nio.charset.CodingErrorAction;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.params.CoreConnectionPNames;
import ch.boye.httpclientandroidlib.params.CoreProtocolPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.ByteArrayBuffer;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;














@NotThreadSafe
@Deprecated
public abstract class AbstractSessionInputBuffer implements SessionInputBuffer, BufferInfo {

    private InputStream instream;
    private byte[] buffer;
    private ByteArrayBuffer linebuffer;
    private Charset charset;
    private boolean ascii;
    private int maxLineLen;
    private int minChunkLimit;
    private HttpTransportMetricsImpl metrics;
    private CodingErrorAction onMalformedCharAction;
    private CodingErrorAction onUnmappableCharAction;

    private int bufferpos;
    private int bufferlen;
    private CharsetDecoder decoder;
    private CharBuffer cbuf;

    public AbstractSessionInputBuffer() {
    }

    






    protected void init(final InputStream instream, final int buffersize, final HttpParams params) {
        Args.notNull(instream, "Input stream");
        Args.notNegative(buffersize, "Buffer size");
        Args.notNull(params, "HTTP parameters");
        this.instream = instream;
        this.buffer = new byte[buffersize];
        this.bufferpos = 0;
        this.bufferlen = 0;
        this.linebuffer = new ByteArrayBuffer(buffersize);
        final String charset = (String) params.getParameter(CoreProtocolPNames.HTTP_ELEMENT_CHARSET);
        this.charset = charset != null ? Charset.forName(charset) : Consts.ASCII;
        this.ascii = this.charset.equals(Consts.ASCII);
        this.decoder = null;
        this.maxLineLen = params.getIntParameter(CoreConnectionPNames.MAX_LINE_LENGTH, -1);
        this.minChunkLimit = params.getIntParameter(CoreConnectionPNames.MIN_CHUNK_LIMIT, 512);
        this.metrics = createTransportMetrics();
        final CodingErrorAction a1 = (CodingErrorAction) params.getParameter(
                CoreProtocolPNames.HTTP_MALFORMED_INPUT_ACTION);
        this.onMalformedCharAction = a1 != null ? a1 : CodingErrorAction.REPORT;
        final CodingErrorAction a2 = (CodingErrorAction) params.getParameter(
                CoreProtocolPNames.HTTP_UNMAPPABLE_INPUT_ACTION);
        this.onUnmappableCharAction = a2 != null? a2 : CodingErrorAction.REPORT;
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
            final int len = this.bufferlen - this.bufferpos;
            if (len > 0) {
                System.arraycopy(this.buffer, this.bufferpos, this.buffer, 0, len);
            }
            this.bufferpos = 0;
            this.bufferlen = len;
        }
        final int l;
        final int off = this.bufferlen;
        final int len = this.buffer.length - off;
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
        int noRead;
        while (!hasBufferedData()) {
            noRead = fillBuffer();
            if (noRead == -1) {
                return -1;
            }
        }
        return this.buffer[this.bufferpos++] & 0xff;
    }

    public int read(final byte[] b, final int off, final int len) throws IOException {
        if (b == null) {
            return 0;
        }
        if (hasBufferedData()) {
            final int chunk = Math.min(len, this.bufferlen - this.bufferpos);
            System.arraycopy(this.buffer, this.bufferpos, b, off, chunk);
            this.bufferpos += chunk;
            return chunk;
        }
        
        
        if (len > this.minChunkLimit) {
            final int read = this.instream.read(b, off, len);
            if (read > 0) {
                this.metrics.incrementBytesTransferred(read);
            }
            return read;
        } else {
            
            while (!hasBufferedData()) {
                final int noRead = fillBuffer();
                if (noRead == -1) {
                    return -1;
                }
            }
            final int chunk = Math.min(len, this.bufferlen - this.bufferpos);
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
        Args.notNull(charbuffer, "Char array buffer");
        int noRead = 0;
        boolean retry = true;
        while (retry) {
            
            final int i = locateLF();
            if (i != -1) {
                
                if (this.linebuffer.isEmpty()) {
                    
                    return lineFromReadBuffer(charbuffer, i);
                }
                retry = false;
                final int len = i + 1 - this.bufferpos;
                this.linebuffer.append(this.buffer, this.bufferpos, len);
                this.bufferpos = i + 1;
            } else {
                
                if (hasBufferedData()) {
                    final int len = this.bufferlen - this.bufferpos;
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
        
        int len = this.linebuffer.length();
        if (len > 0) {
            if (this.linebuffer.byteAt(len - 1) == HTTP.LF) {
                len--;
            }
            
            if (len > 0) {
                if (this.linebuffer.byteAt(len - 1) == HTTP.CR) {
                    len--;
                }
            }
        }
        if (this.ascii) {
            charbuffer.append(this.linebuffer, 0, len);
        } else {
            final ByteBuffer bbuf =  ByteBuffer.wrap(this.linebuffer.buffer(), 0, len);
            len = appendDecoded(charbuffer, bbuf);
        }
        this.linebuffer.clear();
        return len;
    }

    private int lineFromReadBuffer(final CharArrayBuffer charbuffer, final int position)
            throws IOException {
        final int off = this.bufferpos;
        int i = position;
        this.bufferpos = i + 1;
        if (i > off && this.buffer[i - 1] == HTTP.CR) {
            
            i--;
        }
        int len = i - off;
        if (this.ascii) {
            charbuffer.append(this.buffer, off, len);
        } else {
            final ByteBuffer bbuf =  ByteBuffer.wrap(this.buffer, off, len);
            len = appendDecoded(charbuffer, bbuf);
        }
        return len;
    }

    private int appendDecoded(
            final CharArrayBuffer charbuffer, final ByteBuffer bbuf) throws IOException {
        if (!bbuf.hasRemaining()) {
            return 0;
        }
        if (this.decoder == null) {
            this.decoder = this.charset.newDecoder();
            this.decoder.onMalformedInput(this.onMalformedCharAction);
            this.decoder.onUnmappableCharacter(this.onUnmappableCharAction);
        }
        if (this.cbuf == null) {
            this.cbuf = CharBuffer.allocate(1024);
        }
        this.decoder.reset();
        int len = 0;
        while (bbuf.hasRemaining()) {
            final CoderResult result = this.decoder.decode(bbuf, this.cbuf, true);
            len += handleDecodingResult(result, charbuffer, bbuf);
        }
        final CoderResult result = this.decoder.flush(this.cbuf);
        len += handleDecodingResult(result, charbuffer, bbuf);
        this.cbuf.clear();
        return len;
    }

    private int handleDecodingResult(
            final CoderResult result,
            final CharArrayBuffer charbuffer,
            final ByteBuffer bbuf) throws IOException {
        if (result.isError()) {
            result.throwException();
        }
        this.cbuf.flip();
        final int len = this.cbuf.remaining();
        while (this.cbuf.hasRemaining()) {
            charbuffer.append(this.cbuf.get());
        }
        this.cbuf.compact();
        return len;
    }

    public String readLine() throws IOException {
        final CharArrayBuffer charbuffer = new CharArrayBuffer(64);
        final int l = readLine(charbuffer);
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
