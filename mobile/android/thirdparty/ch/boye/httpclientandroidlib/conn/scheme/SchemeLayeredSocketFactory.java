


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.params.HttpParams;









@Deprecated
public interface SchemeLayeredSocketFactory extends SchemeSocketFactory {

    















    Socket createLayeredSocket(
        Socket socket,
        String target,
        int port,
        HttpParams params) throws IOException, UnknownHostException;

}
