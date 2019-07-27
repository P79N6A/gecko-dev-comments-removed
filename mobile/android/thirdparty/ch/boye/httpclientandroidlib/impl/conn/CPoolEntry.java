

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.Date;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;
import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ManagedHttpClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.pool.PoolEntry;




@ThreadSafe
class CPoolEntry extends PoolEntry<HttpRoute, ManagedHttpClientConnection> {

    public HttpClientAndroidLog log;
    private volatile boolean routeComplete;

    public CPoolEntry(
            final HttpClientAndroidLog log,
            final String id,
            final HttpRoute route,
            final ManagedHttpClientConnection conn,
            final long timeToLive, final TimeUnit tunit) {
        super(id, route, conn, timeToLive, tunit);
        this.log = log;
    }

    public void markRouteComplete() {
        this.routeComplete = true;
    }

    public boolean isRouteComplete() {
        return this.routeComplete;
    }

    public void closeConnection() throws IOException {
        final HttpClientConnection conn = getConnection();
        conn.close();
    }

    public void shutdownConnection() throws IOException {
        final HttpClientConnection conn = getConnection();
        conn.shutdown();
    }

    @Override
    public boolean isExpired(final long now) {
        final boolean expired = super.isExpired(now);
        if (expired && this.log.isDebugEnabled()) {
            this.log.debug("Connection " + this + " expired @ " + new Date(getExpiry()));
        }
        return expired;
    }

    @Override
    public boolean isClosed() {
        final HttpClientConnection conn = getConnection();
        return !conn.isOpen();
    }

    @Override
    public void close() {
        try {
            closeConnection();
        } catch (final IOException ex) {
            this.log.debug("I/O error closing connection", ex);
        }
    }

}
