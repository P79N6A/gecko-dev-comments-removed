


























package ch.boye.httpclientandroidlib.conn.socket;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.protocol.HttpContext;






public interface LayeredConnectionSocketFactory extends ConnectionSocketFactory {

    













    Socket createLayeredSocket(
        Socket socket,
        String target,
        int port,
        HttpContext context) throws IOException, UnknownHostException;

}
