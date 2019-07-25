


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;









@Deprecated
public interface LayeredSocketFactory extends SocketFactory {

    
















    Socket createSocket(
        Socket socket,
        String host,
        int port,
        boolean autoClose
    ) throws IOException, UnknownHostException;

}
