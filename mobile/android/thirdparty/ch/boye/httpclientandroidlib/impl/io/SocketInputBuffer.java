


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.net.Socket;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.EofSensor;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;









@NotThreadSafe
@Deprecated
public class SocketInputBuffer extends AbstractSessionInputBuffer implements EofSensor {

    private final Socket socket;

    private boolean eof;

    









    public SocketInputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        super();
        Args.notNull(socket, "Socket");
        this.socket = socket;
        this.eof = false;
        int n = buffersize;
        if (n < 0) {
            n = socket.getReceiveBufferSize();
        }
        if (n < 1024) {
            n = 1024;
        }
        init(socket.getInputStream(), n, params);
    }

    @Override
    protected int fillBuffer() throws IOException {
        final int i = super.fillBuffer();
        this.eof = i == -1;
        return i;
    }

    public boolean isDataAvailable(final int timeout) throws IOException {
        boolean result = hasBufferedData();
        if (!result) {
            final int oldtimeout = this.socket.getSoTimeout();
            try {
                this.socket.setSoTimeout(timeout);
                fillBuffer();
                result = hasBufferedData();
            } finally {
                socket.setSoTimeout(oldtimeout);
            }
        }
        return result;
    }

    public boolean isEof() {
        return this.eof;
    }

}
