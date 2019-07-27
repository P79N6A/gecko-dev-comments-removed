


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.params.HttpParams;




@Deprecated
class SchemeLayeredSocketFactoryAdaptor extends SchemeSocketFactoryAdaptor
    implements SchemeLayeredSocketFactory {

    private final LayeredSocketFactory factory;

    SchemeLayeredSocketFactoryAdaptor(final LayeredSocketFactory factory) {
        super(factory);
        this.factory = factory;
    }

    public Socket createLayeredSocket(
            final Socket socket,
            final String target, final int port,
            final HttpParams params) throws IOException, UnknownHostException {
        return this.factory.createSocket(socket, target, port, true);
    }

}
