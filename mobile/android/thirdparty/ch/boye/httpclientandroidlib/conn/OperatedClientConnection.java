


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.Socket;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpInetConnection;
import ch.boye.httpclientandroidlib.params.HttpParams;










@Deprecated
public interface OperatedClientConnection extends HttpClientConnection, HttpInetConnection {

    











    HttpHost getTargetHost();

    








    boolean isSecure();

    








    Socket getSocket();

    

























    void opening(Socket sock, HttpHost target)
        throws IOException;

    











    void openCompleted(boolean secure, HttpParams params)
        throws IOException;

    




















    void update(Socket sock, HttpHost target,
                boolean secure, HttpParams params)
        throws IOException;

}
