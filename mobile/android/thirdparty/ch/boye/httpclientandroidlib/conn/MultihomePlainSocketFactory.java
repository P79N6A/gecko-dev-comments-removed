


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Arrays;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.conn.scheme.SchemeSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.SocketFactory;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;













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

    














    public Socket connectSocket(Socket sock, String host, int port,
                                InetAddress localAddress, int localPort,
                                HttpParams params)
        throws IOException {

        if (host == null) {
            throw new IllegalArgumentException("Target host may not be null.");
        }
        if (params == null) {
            throw new IllegalArgumentException("Parameters may not be null.");
        }

        if (sock == null)
            sock = createSocket();

        if ((localAddress != null) || (localPort > 0)) {

            
            if (localPort < 0)
                localPort = 0; 

            InetSocketAddress isa =
                new InetSocketAddress(localAddress, localPort);
            sock.bind(isa);
        }

        int timeout = HttpConnectionParams.getConnectionTimeout(params);

        InetAddress[] inetadrs = InetAddress.getAllByName(host);
        List<InetAddress> addresses = new ArrayList<InetAddress>(inetadrs.length);
        addresses.addAll(Arrays.asList(inetadrs));
        Collections.shuffle(addresses);

        IOException lastEx = null;
        for (InetAddress remoteAddress: addresses) {
            try {
                sock.connect(new InetSocketAddress(remoteAddress, port), timeout);
                break;
            } catch (SocketTimeoutException ex) {
                throw new ConnectTimeoutException("Connect to " + remoteAddress + " timed out");
            } catch (IOException ex) {
                
                sock = new Socket();
                
                lastEx = ex;
            }
        }
        if (lastEx != null) {
            throw lastEx;
        }
        return sock;
    } 


    










    public final boolean isSecure(Socket sock)
        throws IllegalArgumentException {

        if (sock == null) {
            throw new IllegalArgumentException("Socket may not be null.");
        }
        
        
        
        if (sock.getClass() != Socket.class) {
            throw new IllegalArgumentException
                ("Socket not created by this factory.");
        }
        
        
        if (sock.isClosed()) {
            throw new IllegalArgumentException("Socket is closed.");
        }

        return false;

    } 

}
