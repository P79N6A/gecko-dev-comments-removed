


























package ch.boye.httpclientandroidlib.conn;

import javax.net.ssl.SSLSession;

import ch.boye.httpclientandroidlib.HttpInetConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;






public interface HttpRoutedConnection extends HttpInetConnection {

    







    boolean isSecure();

    





    HttpRoute getRoute();

    















    SSLSession getSSLSession();

}
