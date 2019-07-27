


























package ch.boye.httpclientandroidlib.conn;

import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
















@Deprecated
public interface ClientConnectionManager {

    




    SchemeRegistry getSchemeRegistry();

    




    ClientConnectionRequest requestConnection(HttpRoute route, Object state);

    















    void releaseConnection(ManagedClientConnection conn, long validDuration, TimeUnit timeUnit);

    













    void closeIdleConnections(long idletime, TimeUnit tunit);

    






    void closeExpiredConnections();

    




    void shutdown();

}
