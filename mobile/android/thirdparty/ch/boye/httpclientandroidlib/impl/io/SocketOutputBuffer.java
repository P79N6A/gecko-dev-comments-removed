


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.net.Socket;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;









@NotThreadSafe
@Deprecated
public class SocketOutputBuffer extends AbstractSessionOutputBuffer {

    









    public SocketOutputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        super();
        Args.notNull(socket, "Socket");
        int n = buffersize;
        if (n < 0) {
            n = socket.getSendBufferSize();
        }
        if (n < 1024) {
            n = 1024;
        }
        init(socket.getOutputStream(), n, params);
    }

}
