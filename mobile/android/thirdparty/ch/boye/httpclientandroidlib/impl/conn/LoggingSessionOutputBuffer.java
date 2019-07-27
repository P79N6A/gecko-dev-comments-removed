

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;






@Immutable
@Deprecated
public class LoggingSessionOutputBuffer implements SessionOutputBuffer {

    
    private final SessionOutputBuffer out;

    
    private final Wire wire;

    private final String charset;

    





    public LoggingSessionOutputBuffer(
            final SessionOutputBuffer out, final Wire wire, final String charset) {
        super();
        this.out = out;
        this.wire = wire;
        this.charset = charset != null ? charset : Consts.ASCII.name();
    }

    public LoggingSessionOutputBuffer(final SessionOutputBuffer out, final Wire wire) {
        this(out, wire, null);
    }

    public void write(final byte[] b, final int off, final int len) throws IOException {
        this.out.write(b,  off,  len);
        if (this.wire.enabled()) {
            this.wire.output(b, off, len);
        }
    }

    public void write(final int b) throws IOException {
        this.out.write(b);
        if (this.wire.enabled()) {
            this.wire.output(b);
        }
    }

    public void write(final byte[] b) throws IOException {
        this.out.write(b);
        if (this.wire.enabled()) {
            this.wire.output(b);
        }
    }

    public void flush() throws IOException {
        this.out.flush();
    }

    public void writeLine(final CharArrayBuffer buffer) throws IOException {
        this.out.writeLine(buffer);
        if (this.wire.enabled()) {
            final String s = new String(buffer.buffer(), 0, buffer.length());
            final String tmp = s + "\r\n";
            this.wire.output(tmp.getBytes(this.charset));
        }
    }

    public void writeLine(final String s) throws IOException {
        this.out.writeLine(s);
        if (this.wire.enabled()) {
            final String tmp = s + "\r\n";
            this.wire.output(tmp.getBytes(this.charset));
        }
    }

    public HttpTransportMetrics getMetrics() {
        return this.out.getMetrics();
    }

}
