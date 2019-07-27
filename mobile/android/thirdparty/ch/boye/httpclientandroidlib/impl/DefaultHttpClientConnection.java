


























package ch.boye.httpclientandroidlib.impl;

import java.io.IOException;
import java.net.Socket;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.params.CoreConnectionPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;








@NotThreadSafe
@Deprecated
public class DefaultHttpClientConnection extends SocketHttpClientConnection {

    public DefaultHttpClientConnection() {
        super();
    }

    @Override
    public void bind(
            final Socket socket,
            final HttpParams params) throws IOException {
        Args.notNull(socket, "Socket");
        Args.notNull(params, "HTTP parameters");
        assertNotOpen();
        socket.setTcpNoDelay(params.getBooleanParameter(CoreConnectionPNames.TCP_NODELAY, true));
        socket.setSoTimeout(params.getIntParameter(CoreConnectionPNames.SO_TIMEOUT, 0));
        socket.setKeepAlive(params.getBooleanParameter(CoreConnectionPNames.SO_KEEPALIVE, false));
        final int linger = params.getIntParameter(CoreConnectionPNames.SO_LINGER, -1);
        if (linger >= 0) {
            socket.setSoLinger(linger > 0, linger);
        }
        super.bind(socket, params);
    }

}
