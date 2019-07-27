


























package ch.boye.httpclientandroidlib.conn;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.concurrent.Cancellable;







public interface ConnectionRequest extends Cancellable {

    





















    HttpClientConnection get(long timeout, TimeUnit tunit)
        throws InterruptedException, ExecutionException, ConnectionPoolTimeoutException;

}
