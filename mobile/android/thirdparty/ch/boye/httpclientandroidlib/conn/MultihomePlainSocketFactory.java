


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.scheme.SocketFactory;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;













@Deprecated
@Immutable
public final class MultihomePlainSocketFactory implements SocketFactory {

    


    private static final
    MultihomePlainSocketFactory DEFAULT_FACTORY = new MultihomePlainSocketFactory();

    



    public static MultihomePlainSocketFactory getSocketFactory() {
        return DEFAULT_FACTORY;
    }

    


    private MultihomePlainSocketFactory() {
        super();
    }


    
    public Socket createSocket() {
        return new Socket();
    }

    














    public Socket connectSocket(final Socket socket, final String host, final int port,
                                final InetAddress localAddress, final int localPort,
                                final HttpParams params)
        throws IOException {
        Args.notNull(host, "Target host");
        Args.notNull(params, "HTTP parameters");

        Socket sock = socket;
        if (sock == null) {
            sock = createSocket();
        }

        if ((localAddress != null) || (localPort > 0)) {
            final InetSocketAddress isa = new InetSocketAddress(localAddress,
                    localPort > 0 ? localPort : 0);
            sock.bind(isa);
        }

        final int timeout = HttpConnectionParams.getConnectionTimeout(params);

        final InetAddress[] inetadrs = InetAddress.getAllByName(host);
        final List<InetAddress> addresses = new ArrayList<InetAddress>(inetadrs.length);
        addresses.addAll(Arrays.asList(inetadrs));
        Collections.shuffle(addresses);

        IOException lastEx = null;
        for (final InetAddress remoteAddress: addresses) {
            try {
                sock.connect(new InetSocketAddress(remoteAddress, port), timeout);
                break;
            } catch (final SocketTimeoutException ex) {
                throw new ConnectTimeoutException("Connect to " + remoteAddress + " timed out");
            } catch (final IOException ex) {
                
                sock = new Socket();
                
                lastEx = ex;
            }
        }
        if (lastEx != null) {
            throw lastEx;
        }
        return sock;
    } 


    










    public final boolean isSecure(final Socket sock)
        throws IllegalArgumentException {

        Args.notNull(sock, "Socket");
        
        
        Asserts.check(!sock.isClosed(), "Socket is closed");
        return false;

    } 

}
