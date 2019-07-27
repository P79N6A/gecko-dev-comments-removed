

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.Date;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.RouteTracker;
import ch.boye.httpclientandroidlib.pool.PoolEntry;






@Deprecated
class HttpPoolEntry extends PoolEntry<HttpRoute, OperatedClientConnection> {

    public HttpClientAndroidLog log;
    private final RouteTracker tracker;

    public HttpPoolEntry(
            final HttpClientAndroidLog log,
            final String id,
            final HttpRoute route,
            final OperatedClientConnection conn,
            final long timeToLive, final TimeUnit tunit) {
        super(id, route, conn, timeToLive, tunit);
        this.log = log;
        this.tracker = new RouteTracker(route);
    }

    @Override
    public boolean isExpired(final long now) {
        final boolean expired = super.isExpired(now);
        if (expired && this.log.isDebugEnabled()) {
            this.log.debug("Connection " + this + " expired @ " + new Date(getExpiry()));
        }
        return expired;
    }

    RouteTracker getTracker() {
        return this.tracker;
    }

    HttpRoute getPlannedRoute() {
        return getRoute();
    }

    HttpRoute getEffectiveRoute() {
        return this.tracker.toRoute();
    }

    @Override
    public boolean isClosed() {
        final OperatedClientConnection conn = getConnection();
        return !conn.isOpen();
    }

    @Override
    public void close() {
        final OperatedClientConnection conn = getConnection();
        try {
            conn.close();
        } catch (final IOException ex) {
            this.log.debug("I/O error closing connection", ex);
        }
    }

}
