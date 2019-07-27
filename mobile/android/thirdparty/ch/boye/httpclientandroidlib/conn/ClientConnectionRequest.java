


























package ch.boye.httpclientandroidlib.conn;

import java.util.concurrent.TimeUnit;








@Deprecated
public interface ClientConnectionRequest {

    






















    ManagedClientConnection getConnection(long timeout, TimeUnit tunit)
        throws InterruptedException, ConnectionPoolTimeoutException;

    



    void abortRequest();

}
