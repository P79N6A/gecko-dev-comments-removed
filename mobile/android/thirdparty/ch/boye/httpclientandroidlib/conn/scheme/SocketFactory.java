


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.conn.ConnectTimeoutException;
import ch.boye.httpclientandroidlib.params.HttpParams;









@Deprecated
public interface SocketFactory {

    








    Socket createSocket()
        throws IOException;

    
























    Socket connectSocket(
        Socket sock,
        String host,
        int port,
        InetAddress localAddress,
        int localPort,
        HttpParams params
    ) throws IOException, UnknownHostException, ConnectTimeoutException;

    




























    boolean isSecure(Socket sock)
        throws IllegalArgumentException;

}
