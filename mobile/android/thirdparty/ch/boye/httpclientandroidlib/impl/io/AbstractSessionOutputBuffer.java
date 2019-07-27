


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CoderResult;
import java.nio.charset.CodingErrorAction;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.BufferInfo;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.params.CoreConnectionPNames;
import ch.boye.httpclientandroidlib.params.CoreProtocolPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.ByteArrayBuffer;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;













@NotThreadSafe
@Deprecated
public abstract class AbstractSessionOutputBuffer implements SessionOutputBuffer, BufferInfo {

    private static final byte[] CRLF = new byte[] {HTTP.CR, HTTP.LF};

    private OutputStream outstream;
    private ByteArrayBuffer buffer;
    private Charset charset;
    private boolean ascii;
    private int minChunkLimit;
    private HttpTransportMetricsImpl metrics;
    private CodingErrorAction onMalformedCharAction;
    private CodingErrorAction onUnmappableCharAction;

    private CharsetEncoder encoder;
    private ByteBuffer bbuf;

    protected AbstractSessionOutputBuffer(
            final OutputStream outstream,
            final int buffersize,
            final Charset charset,
            final int minChunkLimit,
            final CodingErrorAction malformedCharAction,
            final CodingErrorAction unmappableCharAction) {
        super();
        Args.notNull(outstream, "Input stream");
        Args.notNegative(buffersize, "Buffer size");
        this.outstream = outstream;
        this.buffer = new ByteArrayBuffer(buffersize);
        this.charset = charset != null ? charset : Consts.ASCII;
        this.ascii = this.charset.equals(Consts.ASCII);
        this.encoder = null;
        this.minChunkLimit = minChunkLimit >= 0 ? minChunkLimit : 512;
        this.metrics = createTransportMetrics();
        this.onMalformedCharAction = malformedCharAction != null ? malformedCharAction :
            CodingErrorAction.REPORT;
        this.onUnmappableCharAction = unmappableCharAction != null? unmappableCharAction :
            CodingErrorAction.REPORT;
    }

    public AbstractSessionOutputBuffer() {
    }

    protected void init(final OutputStream outstream, final int buffersize, final HttpParams params) {
        Args.notNull(outstream, "Input stream");
        Args.notNegative(buffersize, "Buffer size");
        Args.notNull(params, "HTTP parameters");
        this.outstream = outstream;
        this.buffer = new ByteArrayBuffer(buffersize);
        final String charset = (String) params.getParameter(CoreProtocolPNames.HTTP_ELEMENT_CHARSET);
        this.charset = charset != null ? Charset.forName(charset) : Consts.ASCII;
        this.ascii = this.charset.equals(Consts.ASCII);
        this.encoder = null;
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
        return this.buffer.capacity();
    }

    


    public int length() {
        return this.buffer.length();
    }

    


    public int available() {
        return capacity() - length();
    }

    protected void flushBuffer() throws IOException {
        final int len = this.buffer.length();
        if (len > 0) {
            this.outstream.write(this.buffer.buffer(), 0, len);
            this.buffer.clear();
            this.metrics.incrementBytesTransferred(len);
        }
    }

    public void flush() throws IOException {
        flushBuffer();
        this.outstream.flush();
    }

    public void write(final byte[] b, final int off, final int len) throws IOException {
        if (b == null) {
            return;
        }
        
        
        
        if (len > this.minChunkLimit || len > this.buffer.capacity()) {
            
            flushBuffer();
            
            this.outstream.write(b, off, len);
            this.metrics.incrementBytesTransferred(len);
        } else {
            
            final int freecapacity = this.buffer.capacity() - this.buffer.length();
            if (len > freecapacity) {
                
                flushBuffer();
            }
            
            this.buffer.append(b, off, len);
        }
    }

    public void write(final byte[] b) throws IOException {
        if (b == null) {
            return;
        }
        write(b, 0, b.length);
    }

    public void write(final int b) throws IOException {
        if (this.buffer.isFull()) {
            flushBuffer();
        }
        this.buffer.append(b);
    }

    








    public void writeLine(final String s) throws IOException {
        if (s == null) {
            return;
        }
        if (s.length() > 0) {
            if (this.ascii) {
                for (int i = 0; i < s.length(); i++) {
                    write(s.charAt(i));
                }
            } else {
                final CharBuffer cbuf = CharBuffer.wrap(s);
                writeEncoded(cbuf);
            }
        }
        write(CRLF);
    }

    








    public void writeLine(final CharArrayBuffer charbuffer) throws IOException {
        if (charbuffer == null) {
            return;
        }
        if (this.ascii) {
            int off = 0;
            int remaining = charbuffer.length();
            while (remaining > 0) {
                int chunk = this.buffer.capacity() - this.buffer.length();
                chunk = Math.min(chunk, remaining);
                if (chunk > 0) {
                    this.buffer.append(charbuffer, off, chunk);
                }
                if (this.buffer.isFull()) {
                    flushBuffer();
                }
                off += chunk;
                remaining -= chunk;
            }
        } else {
            final CharBuffer cbuf = CharBuffer.wrap(charbuffer.buffer(), 0, charbuffer.length());
            writeEncoded(cbuf);
        }
        write(CRLF);
    }

    private void writeEncoded(final CharBuffer cbuf) throws IOException {
        if (!cbuf.hasRemaining()) {
            return;
        }
        if (this.encoder == null) {
            this.encoder = this.charset.newEncoder();
            this.encoder.onMalformedInput(this.onMalformedCharAction);
            this.encoder.onUnmappableCharacter(this.onUnmappableCharAction);
        }
        if (this.bbuf == null) {
            this.bbuf = ByteBuffer.allocate(1024);
        }
        this.encoder.reset();
        while (cbuf.hasRemaining()) {
            final CoderResult result = this.encoder.encode(cbuf, this.bbuf, true);
            handleEncodingResult(result);
        }
        final CoderResult result = this.encoder.flush(this.bbuf);
        handleEncodingResult(result);
        this.bbuf.clear();
    }

    private void handleEncodingResult(final CoderResult result) throws IOException {
        if (result.isError()) {
            result.throwException();
        }
        this.bbuf.flip();
        while (this.bbuf.hasRemaining()) {
            write(this.bbuf.get());
        }
        this.bbuf.compact();
    }

    public HttpTransportMetrics getMetrics() {
        return this.metrics;
    }

}
