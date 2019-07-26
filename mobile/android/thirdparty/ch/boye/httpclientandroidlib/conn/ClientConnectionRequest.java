


























package ch.boye.httpclientandroidlib.conn;

import java.util.concurrent.TimeUnit;






public interface ClientConnectionRequest {

    






















    ManagedClientConnection getConnection(long timeout, TimeUnit tunit)
        throws InterruptedException, ConnectionPoolTimeoutException;

    



    void abortRequest();

}
