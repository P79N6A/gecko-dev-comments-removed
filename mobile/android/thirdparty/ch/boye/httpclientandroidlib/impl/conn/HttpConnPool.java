

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.pool.AbstractConnPool;
import ch.boye.httpclientandroidlib.pool.ConnFactory;






@Deprecated
class HttpConnPool extends AbstractConnPool<HttpRoute, OperatedClientConnection, HttpPoolEntry> {

    private static final AtomicLong COUNTER = new AtomicLong();

    public HttpClientAndroidLog log;
    private final long timeToLive;
    private final TimeUnit tunit;

    public HttpConnPool(final HttpClientAndroidLog log,
            final ClientConnectionOperator connOperator,
            final int defaultMaxPerRoute, final int maxTotal,
            final long timeToLive, final TimeUnit tunit) {
        super(new InternalConnFactory(connOperator), defaultMaxPerRoute, maxTotal);
        this.log = log;
        this.timeToLive = timeToLive;
        this.tunit = tunit;
    }

    @Override
    protected HttpPoolEntry createEntry(final HttpRoute route, final OperatedClientConnection conn) {
        final String id = Long.toString(COUNTER.getAndIncrement());
        return new HttpPoolEntry(this.log, id, route, conn, this.timeToLive, this.tunit);
    }

    static class InternalConnFactory implements ConnFactory<HttpRoute, OperatedClientConnection> {

        private final ClientConnectionOperator connOperator;

        InternalConnFactory(final ClientConnectionOperator connOperator) {
            this.connOperator = connOperator;
        }

        public OperatedClientConnection create(final HttpRoute route) throws IOException {
            return connOperator.createConnection();
        }

    }

}
