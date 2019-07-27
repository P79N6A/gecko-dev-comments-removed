


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;



















@ThreadSafe
@Deprecated
public class BasicClientConnectionManager implements ClientConnectionManager {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private static final AtomicLong COUNTER = new AtomicLong();

    
    public final static String MISUSE_MESSAGE =
    "Invalid use of BasicClientConnManager: connection still allocated.\n" +
    "Make sure to release the connection before allocating another one.";

    
    private final SchemeRegistry schemeRegistry;

    
    private final ClientConnectionOperator connOperator;

    
    @GuardedBy("this")
    private HttpPoolEntry poolEntry;

    
    @GuardedBy("this")
    private ManagedClientConnectionImpl conn;

    
    @GuardedBy("this")
    private volatile boolean shutdown;

    




    public BasicClientConnectionManager(final SchemeRegistry schreg) {
        Args.notNull(schreg, "Scheme registry");
        this.schemeRegistry = schreg;
        this.connOperator = createConnectionOperator(schreg);
    }

    public BasicClientConnectionManager() {
        this(SchemeRegistryFactory.createDefault());
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            shutdown();
        } finally { 
            super.finalize();
        }
    }

    public SchemeRegistry getSchemeRegistry() {
        return this.schemeRegistry;
    }

    protected ClientConnectionOperator createConnectionOperator(final SchemeRegistry schreg) {
        return new DefaultClientConnectionOperator(schreg);
    }

    public final ClientConnectionRequest requestConnection(
            final HttpRoute route,
            final Object state) {

        return new ClientConnectionRequest() {

            public void abortRequest() {
                
            }

            public ManagedClientConnection getConnection(
                    final long timeout, final TimeUnit tunit) {
                return BasicClientConnectionManager.this.getConnection(
                        route, state);
            }

        };
    }

    private void assertNotShutdown() {
        Asserts.check(!this.shutdown, "Connection manager has been shut down");
    }

    ManagedClientConnection getConnection(final HttpRoute route, final Object state) {
        Args.notNull(route, "Route");
        synchronized (this) {
            assertNotShutdown();
            if (this.log.isDebugEnabled()) {
                this.log.debug("Get connection for route " + route);
            }
            Asserts.check(this.conn == null, MISUSE_MESSAGE);
            if (this.poolEntry != null && !this.poolEntry.getPlannedRoute().equals(route)) {
                this.poolEntry.close();
                this.poolEntry = null;
            }
            if (this.poolEntry == null) {
                final String id = Long.toString(COUNTER.getAndIncrement());
                final OperatedClientConnection conn = this.connOperator.createConnection();
                this.poolEntry = new HttpPoolEntry(this.log, id, route, conn, 0, TimeUnit.MILLISECONDS);
            }
            final long now = System.currentTimeMillis();
            if (this.poolEntry.isExpired(now)) {
                this.poolEntry.close();
                this.poolEntry.getTracker().reset();
            }
            this.conn = new ManagedClientConnectionImpl(this, this.connOperator, this.poolEntry);
            return this.conn;
        }
    }

    private void shutdownConnection(final HttpClientConnection conn) {
        try {
            conn.shutdown();
        } catch (final IOException iox) {
            if (this.log.isDebugEnabled()) {
                this.log.debug("I/O exception shutting down connection", iox);
            }
        }
    }

    public void releaseConnection(final ManagedClientConnection conn, final long keepalive, final TimeUnit tunit) {
        Args.check(conn instanceof ManagedClientConnectionImpl, "Connection class mismatch, " +
            "connection not obtained from this manager");
        final ManagedClientConnectionImpl managedConn = (ManagedClientConnectionImpl) conn;
        synchronized (managedConn) {
            if (this.log.isDebugEnabled()) {
                this.log.debug("Releasing connection " + conn);
            }
            if (managedConn.getPoolEntry() == null) {
                return; 
            }
            final ClientConnectionManager manager = managedConn.getManager();
            Asserts.check(manager == this, "Connection not obtained from this manager");
            synchronized (this) {
                if (this.shutdown) {
                    shutdownConnection(managedConn);
                    return;
                }
                try {
                    if (managedConn.isOpen() && !managedConn.isMarkedReusable()) {
                        shutdownConnection(managedConn);
                    }
                    if (managedConn.isMarkedReusable()) {
                        this.poolEntry.updateExpiry(keepalive, tunit != null ? tunit : TimeUnit.MILLISECONDS);
                        if (this.log.isDebugEnabled()) {
                            final String s;
                            if (keepalive > 0) {
                                s = "for " + keepalive + " " + tunit;
                            } else {
                                s = "indefinitely";
                            }
                            this.log.debug("Connection can be kept alive " + s);
                        }
                    }
                } finally {
                    managedConn.detach();
                    this.conn = null;
                    if (this.poolEntry.isClosed()) {
                        this.poolEntry = null;
                    }
                }
            }
        }
    }

    public void closeExpiredConnections() {
        synchronized (this) {
            assertNotShutdown();
            final long now = System.currentTimeMillis();
            if (this.poolEntry != null && this.poolEntry.isExpired(now)) {
                this.poolEntry.close();
                this.poolEntry.getTracker().reset();
            }
        }
    }

    public void closeIdleConnections(final long idletime, final TimeUnit tunit) {
        Args.notNull(tunit, "Time unit");
        synchronized (this) {
            assertNotShutdown();
            long time = tunit.toMillis(idletime);
            if (time < 0) {
                time = 0;
            }
            final long deadline = System.currentTimeMillis() - time;
            if (this.poolEntry != null && this.poolEntry.getUpdated() <= deadline) {
                this.poolEntry.close();
                this.poolEntry.getTracker().reset();
            }
        }
    }

    public void shutdown() {
        synchronized (this) {
            this.shutdown = true;
            try {
                if (this.poolEntry != null) {
                    this.poolEntry.close();
                }
            } finally {
                this.poolEntry = null;
                this.conn = null;
            }
        }
    }

}
