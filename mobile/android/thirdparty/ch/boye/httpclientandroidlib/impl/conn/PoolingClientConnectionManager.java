

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;
import ch.boye.httpclientandroidlib.conn.DnsResolver;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.pool.ConnPoolControl;
import ch.boye.httpclientandroidlib.pool.PoolStats;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;





















@Deprecated
@ThreadSafe
public class PoolingClientConnectionManager implements ClientConnectionManager, ConnPoolControl<HttpRoute> {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final SchemeRegistry schemeRegistry;

    private final HttpConnPool pool;

    private final ClientConnectionOperator operator;

    
    private final DnsResolver dnsResolver;

    public PoolingClientConnectionManager(final SchemeRegistry schreg) {
        this(schreg, -1, TimeUnit.MILLISECONDS);
    }

    public PoolingClientConnectionManager(final SchemeRegistry schreg,final DnsResolver dnsResolver) {
        this(schreg, -1, TimeUnit.MILLISECONDS,dnsResolver);
    }

    public PoolingClientConnectionManager() {
        this(SchemeRegistryFactory.createDefault());
    }

    public PoolingClientConnectionManager(
            final SchemeRegistry schemeRegistry,
            final long timeToLive, final TimeUnit tunit) {
        this(schemeRegistry, timeToLive, tunit, new SystemDefaultDnsResolver());
    }

    public PoolingClientConnectionManager(final SchemeRegistry schemeRegistry,
                final long timeToLive, final TimeUnit tunit,
                final DnsResolver dnsResolver) {
        super();
        Args.notNull(schemeRegistry, "Scheme registry");
        Args.notNull(dnsResolver, "DNS resolver");
        this.schemeRegistry = schemeRegistry;
        this.dnsResolver  = dnsResolver;
        this.operator = createConnectionOperator(schemeRegistry);
        this.pool = new HttpConnPool(this.log, this.operator, 2, 20, timeToLive, tunit);
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            shutdown();
        } finally {
            super.finalize();
        }
    }

    











    protected ClientConnectionOperator createConnectionOperator(final SchemeRegistry schreg) {
            return new DefaultClientConnectionOperator(schreg, this.dnsResolver);
    }

    public SchemeRegistry getSchemeRegistry() {
        return this.schemeRegistry;
    }

    private String format(final HttpRoute route, final Object state) {
        final StringBuilder buf = new StringBuilder();
        buf.append("[route: ").append(route).append("]");
        if (state != null) {
            buf.append("[state: ").append(state).append("]");
        }
        return buf.toString();
    }

    private String formatStats(final HttpRoute route) {
        final StringBuilder buf = new StringBuilder();
        final PoolStats totals = this.pool.getTotalStats();
        final PoolStats stats = this.pool.getStats(route);
        buf.append("[total kept alive: ").append(totals.getAvailable()).append("; ");
        buf.append("route allocated: ").append(stats.getLeased() + stats.getAvailable());
        buf.append(" of ").append(stats.getMax()).append("; ");
        buf.append("total allocated: ").append(totals.getLeased() + totals.getAvailable());
        buf.append(" of ").append(totals.getMax()).append("]");
        return buf.toString();
    }

    private String format(final HttpPoolEntry entry) {
        final StringBuilder buf = new StringBuilder();
        buf.append("[id: ").append(entry.getId()).append("]");
        buf.append("[route: ").append(entry.getRoute()).append("]");
        final Object state = entry.getState();
        if (state != null) {
            buf.append("[state: ").append(state).append("]");
        }
        return buf.toString();
    }

    public ClientConnectionRequest requestConnection(
            final HttpRoute route,
            final Object state) {
        Args.notNull(route, "HTTP route");
        if (this.log.isDebugEnabled()) {
            this.log.debug("Connection request: " + format(route, state) + formatStats(route));
        }
        final Future<HttpPoolEntry> future = this.pool.lease(route, state);

        return new ClientConnectionRequest() {

            public void abortRequest() {
                future.cancel(true);
            }

            public ManagedClientConnection getConnection(
                    final long timeout,
                    final TimeUnit tunit) throws InterruptedException, ConnectionPoolTimeoutException {
                return leaseConnection(future, timeout, tunit);
            }

        };

    }

    ManagedClientConnection leaseConnection(
            final Future<HttpPoolEntry> future,
            final long timeout,
            final TimeUnit tunit) throws InterruptedException, ConnectionPoolTimeoutException {
        final HttpPoolEntry entry;
        try {
            entry = future.get(timeout, tunit);
            if (entry == null || future.isCancelled()) {
                throw new InterruptedException();
            }
            Asserts.check(entry.getConnection() != null, "Pool entry with no connection");
            if (this.log.isDebugEnabled()) {
                this.log.debug("Connection leased: " + format(entry) + formatStats(entry.getRoute()));
            }
            return new ManagedClientConnectionImpl(this, this.operator, entry);
        } catch (final ExecutionException ex) {
            Throwable cause = ex.getCause();
            if (cause == null) {
                cause = ex;
            }
            this.log.error("Unexpected exception leasing connection from pool", cause);
            
            throw new InterruptedException();
        } catch (final TimeoutException ex) {
            throw new ConnectionPoolTimeoutException("Timeout waiting for connection from pool");
        }
    }

    public void releaseConnection(
            final ManagedClientConnection conn, final long keepalive, final TimeUnit tunit) {

        Args.check(conn instanceof ManagedClientConnectionImpl, "Connection class mismatch, " +
            "connection not obtained from this manager");
        final ManagedClientConnectionImpl managedConn = (ManagedClientConnectionImpl) conn;
        Asserts.check(managedConn.getManager() == this, "Connection not obtained from this manager");
        synchronized (managedConn) {
            final HttpPoolEntry entry = managedConn.detach();
            if (entry == null) {
                return;
            }
            try {
                if (managedConn.isOpen() && !managedConn.isMarkedReusable()) {
                    try {
                        managedConn.shutdown();
                    } catch (final IOException iox) {
                        if (this.log.isDebugEnabled()) {
                            this.log.debug("I/O exception shutting down released connection", iox);
                        }
                    }
                }
                
                if (managedConn.isMarkedReusable()) {
                    entry.updateExpiry(keepalive, tunit != null ? tunit : TimeUnit.MILLISECONDS);
                    if (this.log.isDebugEnabled()) {
                        final String s;
                        if (keepalive > 0) {
                            s = "for " + keepalive + " " + tunit;
                        } else {
                            s = "indefinitely";
                        }
                        this.log.debug("Connection " + format(entry) + " can be kept alive " + s);
                    }
                }
            } finally {
                this.pool.release(entry, managedConn.isMarkedReusable());
            }
            if (this.log.isDebugEnabled()) {
                this.log.debug("Connection released: " + format(entry) + formatStats(entry.getRoute()));
            }
        }
    }

    public void shutdown() {
        this.log.debug("Connection manager is shutting down");
        try {
            this.pool.shutdown();
        } catch (final IOException ex) {
            this.log.debug("I/O exception shutting down connection manager", ex);
        }
        this.log.debug("Connection manager shut down");
    }

    public void closeIdleConnections(final long idleTimeout, final TimeUnit tunit) {
        if (this.log.isDebugEnabled()) {
            this.log.debug("Closing connections idle longer than " + idleTimeout + " " + tunit);
        }
        this.pool.closeIdle(idleTimeout, tunit);
    }

    public void closeExpiredConnections() {
        this.log.debug("Closing expired connections");
        this.pool.closeExpired();
    }

    public int getMaxTotal() {
        return this.pool.getMaxTotal();
    }

    public void setMaxTotal(final int max) {
        this.pool.setMaxTotal(max);
    }

    public int getDefaultMaxPerRoute() {
        return this.pool.getDefaultMaxPerRoute();
    }

    public void setDefaultMaxPerRoute(final int max) {
        this.pool.setDefaultMaxPerRoute(max);
    }

    public int getMaxPerRoute(final HttpRoute route) {
        return this.pool.getMaxPerRoute(route);
    }

    public void setMaxPerRoute(final HttpRoute route, final int max) {
        this.pool.setMaxPerRoute(route, max);
    }

    public PoolStats getTotalStats() {
        return this.pool.getTotalStats();
    }

    public PoolStats getStats(final HttpRoute route) {
        return this.pool.getStats(route);
    }

}

