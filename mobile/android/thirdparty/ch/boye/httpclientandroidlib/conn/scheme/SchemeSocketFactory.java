


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.conn.ConnectTimeoutException;
import ch.boye.httpclientandroidlib.params.HttpParams;









@Deprecated
public interface SchemeSocketFactory {

    















    Socket createSocket(HttpParams params) throws IOException;

    






























    Socket connectSocket(
        Socket sock,
        InetSocketAddress remoteAddress,
        InetSocketAddress localAddress,
        HttpParams params) throws IOException, UnknownHostException, ConnectTimeoutException;

    























    boolean isSecure(Socket sock) throws IllegalArgumentException;

}
