


























package ch.boye.httpclientandroidlib.conn.socket;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.protocol.HttpContext;






public interface ConnectionSocketFactory {

    








    Socket createSocket(HttpContext context) throws IOException;

    
















    Socket connectSocket(
        int connectTimeout,
        Socket sock,
        HttpHost host,
        InetSocketAddress remoteAddress,
        InetSocketAddress localAddress,
        HttpContext context) throws IOException;

}
