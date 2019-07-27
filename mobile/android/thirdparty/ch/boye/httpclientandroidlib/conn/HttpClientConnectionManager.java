

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.protocol.HttpContext;















public interface HttpClientConnectionManager {

    

























    ConnectionRequest requestConnection(HttpRoute route, Object state);

    












    void releaseConnection(
            HttpClientConnection conn, Object newState, long validDuration, TimeUnit timeUnit);

    










    void connect(
            HttpClientConnection conn,
            HttpRoute route,
            int connectTimeout,
            HttpContext context) throws IOException;

    









    void upgrade(
            HttpClientConnection conn,
            HttpRoute route,
            HttpContext context) throws IOException;

    








    void routeComplete(
            HttpClientConnection conn,
            HttpRoute route,
            HttpContext context) throws IOException;

    














    void closeIdleConnections(long idletime, TimeUnit tunit);

    







    void closeExpiredConnections();

    




    void shutdown();

}
