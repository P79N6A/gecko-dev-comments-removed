


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.Socket;

import javax.net.ssl.SSLSession;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpInetConnection;









public interface ManagedHttpClientConnection extends HttpClientConnection, HttpInetConnection {

    



    String getId();

    







    void bind(Socket socket) throws IOException;

    


    Socket getSocket();

    








    SSLSession getSSLSession();

}
