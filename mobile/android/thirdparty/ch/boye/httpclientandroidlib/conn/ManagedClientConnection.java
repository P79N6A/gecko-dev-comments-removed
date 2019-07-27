


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLSession;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;









@Deprecated
public interface ManagedClientConnection extends
    HttpClientConnection, HttpRoutedConnection, ManagedHttpClientConnection, ConnectionReleaseTrigger {

    







    boolean isSecure();

    





    HttpRoute getRoute();

    















    SSLSession getSSLSession();

    









    void open(HttpRoute route, HttpContext context, HttpParams params)
        throws IOException;

    
















    void tunnelTarget(boolean secure, HttpParams params)
        throws IOException;

    



















    void tunnelProxy(HttpHost next, boolean secure, HttpParams params)
        throws IOException;

    












    void layerProtocol(HttpContext context, HttpParams params)
        throws IOException;

    













    void markReusable();

    












    void unmarkReusable();

    








    boolean isMarkedReusable();

    





    void setState(Object state);

    




    Object getState();

    







    void setIdleDuration(long duration, TimeUnit unit);

}
