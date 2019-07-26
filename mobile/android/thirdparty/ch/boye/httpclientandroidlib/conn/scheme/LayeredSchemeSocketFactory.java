


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;







public interface LayeredSchemeSocketFactory extends SchemeSocketFactory {

    
















    Socket createLayeredSocket(
        Socket socket,
        String target,
        int port,
        boolean autoClose
    ) throws IOException, UnknownHostException;

}
