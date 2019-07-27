

























package ch.boye.httpclientandroidlib.impl.conn;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ManagedHttpClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.pool.AbstractConnPool;
import ch.boye.httpclientandroidlib.pool.ConnFactory;




@ThreadSafe
class CPool extends AbstractConnPool<HttpRoute, ManagedHttpClientConnection, CPoolEntry> {

    private static final AtomicLong COUNTER = new AtomicLong();

    public HttpClientAndroidLog log = new HttpClientAndroidLog(CPool.class);
    private final long timeToLive;
    private final TimeUnit tunit;

    public CPool(
            final ConnFactory<HttpRoute, ManagedHttpClientConnection> connFactory,
            final int defaultMaxPerRoute, final int maxTotal,
            final long timeToLive, final TimeUnit tunit) {
        super(connFactory, defaultMaxPerRoute, maxTotal);
        this.timeToLive = timeToLive;
        this.tunit = tunit;
    }

    @Override
    protected CPoolEntry createEntry(final HttpRoute route, final ManagedHttpClientConnection conn) {
        final String id = Long.toString(COUNTER.getAndIncrement());
        return new CPoolEntry(this.log, id, route, conn, this.timeToLive, this.tunit);
    }

}
