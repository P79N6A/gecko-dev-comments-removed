

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class Wire {

    public HttpClientAndroidLog log;
    private final String id;

    


    public Wire(final HttpClientAndroidLog log, final String id) {
        this.log = log;
        this.id = id;
    }

    public Wire(final HttpClientAndroidLog log) {
        this(log, "");
    }

    private void wire(final String header, final InputStream instream)
      throws IOException {
        final StringBuilder buffer = new StringBuilder();
        int ch;
        while ((ch = instream.read()) != -1) {
            if (ch == 13) {
                buffer.append("[\\r]");
            } else if (ch == 10) {
                    buffer.append("[\\n]\"");
                    buffer.insert(0, "\"");
                    buffer.insert(0, header);
                    log.debug(id + " " + buffer.toString());
                    buffer.setLength(0);
            } else if ((ch < 32) || (ch > 127)) {
                buffer.append("[0x");
                buffer.append(Integer.toHexString(ch));
                buffer.append("]");
            } else {
                buffer.append((char) ch);
            }
        }
        if (buffer.length() > 0) {
            buffer.append('\"');
            buffer.insert(0, '\"');
            buffer.insert(0, header);
            log.debug(id + " " + buffer.toString());
        }
    }


    public boolean enabled() {
        return log.isDebugEnabled();
    }

    public void output(final InputStream outstream)
      throws IOException {
        Args.notNull(outstream, "Output");
        wire(">> ", outstream);
    }

    public void input(final InputStream instream)
      throws IOException {
        Args.notNull(instream, "Input");
        wire("<< ", instream);
    }

    public void output(final byte[] b, final int off, final int len)
      throws IOException {
        Args.notNull(b, "Output");
        wire(">> ", new ByteArrayInputStream(b, off, len));
    }

    public void input(final byte[] b, final int off, final int len)
      throws IOException {
        Args.notNull(b, "Input");
        wire("<< ", new ByteArrayInputStream(b, off, len));
    }

    public void output(final byte[] b)
      throws IOException {
        Args.notNull(b, "Output");
        wire(">> ", new ByteArrayInputStream(b));
    }

    public void input(final byte[] b)
      throws IOException {
        Args.notNull(b, "Input");
        wire("<< ", new ByteArrayInputStream(b));
    }

    public void output(final int b)
      throws IOException {
        output(new byte[] {(byte) b});
    }

    public void input(final int b)
      throws IOException {
        input(new byte[] {(byte) b});
    }

    public void output(final String s)
      throws IOException {
        Args.notNull(s, "Output");
        output(s.getBytes());
    }

    public void input(final String s)
      throws IOException {
        Args.notNull(s, "Input");
        input(s.getBytes());
    }
}
