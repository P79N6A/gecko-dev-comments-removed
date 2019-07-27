

























package ch.boye.httpclientandroidlib.impl.pool;

import java.util.concurrent.atomic.AtomicLong;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.config.ConnectionConfig;
import ch.boye.httpclientandroidlib.config.SocketConfig;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.pool.AbstractConnPool;
import ch.boye.httpclientandroidlib.pool.ConnFactory;











@SuppressWarnings("deprecation")
@ThreadSafe
public class BasicConnPool extends AbstractConnPool<HttpHost, HttpClientConnection, BasicPoolEntry> {

    private static final AtomicLong COUNTER = new AtomicLong();

    public BasicConnPool(final ConnFactory<HttpHost, HttpClientConnection> connFactory) {
        super(connFactory, 2, 20);
    }

    


    @Deprecated
    public BasicConnPool(final HttpParams params) {
        super(new BasicConnFactory(params), 2, 20);
    }

    


    public BasicConnPool(final SocketConfig sconfig, final ConnectionConfig cconfig) {
        super(new BasicConnFactory(sconfig, cconfig), 2, 20);
    }

    


    public BasicConnPool() {
        super(new BasicConnFactory(SocketConfig.DEFAULT, ConnectionConfig.DEFAULT), 2, 20);
    }

    @Override
    protected BasicPoolEntry createEntry(
            final HttpHost host,
            final HttpClientConnection conn) {
        return new BasicPoolEntry(Long.toString(COUNTER.getAndIncrement()), host, conn);
    }

}
