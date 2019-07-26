


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

@Deprecated
class LayeredSchemeSocketFactoryAdaptor extends SchemeSocketFactoryAdaptor
    implements LayeredSchemeSocketFactory {

    private final LayeredSocketFactory factory;

    LayeredSchemeSocketFactoryAdaptor(final LayeredSocketFactory factory) {
        super(factory);
        this.factory = factory;
    }

    public Socket createLayeredSocket(
            final Socket socket,
            final String target, int port,
            boolean autoClose) throws IOException, UnknownHostException {
        return this.factory.createSocket(socket, target, port, autoClose);
    }

}
