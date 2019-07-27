

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;








@Deprecated
public interface PoolEntryRequest {

    















    BasicPoolEntry getPoolEntry(
            long timeout,
            TimeUnit tunit) throws InterruptedException, ConnectionPoolTimeoutException;

    



    void abortRequest();

}
