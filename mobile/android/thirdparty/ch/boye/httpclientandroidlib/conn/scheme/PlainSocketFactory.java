


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.ConnectTimeoutException;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;








@Immutable
@Deprecated
public class PlainSocketFactory implements SocketFactory, SchemeSocketFactory {

    private final HostNameResolver nameResolver;

    




    public static PlainSocketFactory getSocketFactory() {
        return new PlainSocketFactory();
    }

    


    @Deprecated
    public PlainSocketFactory(final HostNameResolver nameResolver) {
        super();
        this.nameResolver = nameResolver;
    }

    public PlainSocketFactory() {
        super();
        this.nameResolver = null;
    }

    






    public Socket createSocket(final HttpParams params) {
        return new Socket();
    }

    public Socket createSocket() {
        return new Socket();
    }

    


    public Socket connectSocket(
            final Socket socket,
            final InetSocketAddress remoteAddress,
            final InetSocketAddress localAddress,
            final HttpParams params) throws IOException, ConnectTimeoutException {
        Args.notNull(remoteAddress, "Remote address");
        Args.notNull(params, "HTTP parameters");
        Socket sock = socket;
        if (sock == null) {
            sock = createSocket();
        }
        if (localAddress != null) {
            sock.setReuseAddress(HttpConnectionParams.getSoReuseaddr(params));
            sock.bind(localAddress);
        }
        final int connTimeout = HttpConnectionParams.getConnectionTimeout(params);
        final int soTimeout = HttpConnectionParams.getSoTimeout(params);

        try {
            sock.setSoTimeout(soTimeout);
            sock.connect(remoteAddress, connTimeout);
        } catch (final SocketTimeoutException ex) {
            throw new ConnectTimeoutException("Connect to " + remoteAddress + " timed out");
        }
        return sock;
    }

    








    public final boolean isSecure(final Socket sock) {
        return false;
    }

    


    @Deprecated
    public Socket connectSocket(
            final Socket socket,
            final String host, final int port,
            final InetAddress localAddress, final int localPort,
            final HttpParams params) throws IOException, UnknownHostException, ConnectTimeoutException {
        InetSocketAddress local = null;
        if (localAddress != null || localPort > 0) {
            local = new InetSocketAddress(localAddress, localPort > 0 ? localPort : 0);
        }
        final InetAddress remoteAddress;
        if (this.nameResolver != null) {
            remoteAddress = this.nameResolver.resolve(host);
        } else {
            remoteAddress = InetAddress.getByName(host);
        }
        final InetSocketAddress remote = new InetSocketAddress(remoteAddress, port);
        return connectSocket(socket, remote, local, params);
    }

}
