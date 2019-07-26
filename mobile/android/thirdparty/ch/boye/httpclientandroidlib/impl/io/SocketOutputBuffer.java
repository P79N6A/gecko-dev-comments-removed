


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.net.Socket;

import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.params.HttpParams;












public class SocketOutputBuffer extends AbstractSessionOutputBuffer {

    









    public SocketOutputBuffer(
            final Socket socket,
            int buffersize,
            final HttpParams params) throws IOException {
        super();
        if (socket == null) {
            throw new IllegalArgumentException("Socket may not be null");
        }
        if (buffersize < 0) {
            buffersize = socket.getSendBufferSize();
        }
        if (buffersize < 1024) {
            buffersize = 1024;
        }
        init(socket.getOutputStream(), buffersize, params);
    }

}
