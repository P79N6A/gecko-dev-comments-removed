


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.RouteTracker;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;

















@ThreadSafe
@Deprecated
public class SingleClientConnManager implements ClientConnectionManager {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    
    public final static String MISUSE_MESSAGE =
    "Invalid use of SingleClientConnManager: connection still allocated.\n" +
    "Make sure to release the connection before allocating another one.";

    
    protected final SchemeRegistry schemeRegistry;

    
    protected final ClientConnectionOperator connOperator;

    
    protected final boolean alwaysShutDown;

    
    @GuardedBy("this")
    protected volatile PoolEntry uniquePoolEntry;

    
    @GuardedBy("this")
    protected volatile ConnAdapter managedConn;

    
    @GuardedBy("this")
    protected volatile long lastReleaseTime;

    
    @GuardedBy("this")
    protected volatile long connectionExpiresTime;

    
    protected volatile boolean isShutDown;

    







    @Deprecated
    public SingleClientConnManager(final HttpParams params,
                                   final SchemeRegistry schreg) {
        this(schreg);
    }
    




    public SingleClientConnManager(final SchemeRegistry schreg) {
        Args.notNull(schreg, "Scheme registry");
        this.schemeRegistry  = schreg;
        this.connOperator    = createConnectionOperator(schreg);
        this.uniquePoolEntry = new PoolEntry();
        this.managedConn     = null;
        this.lastReleaseTime = -1L;
        this.alwaysShutDown  = false; 
        this.isShutDown      = false;
    }

    


    public SingleClientConnManager() {
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

    











    protected ClientConnectionOperator
        createConnectionOperator(final SchemeRegistry schreg) {
        return new DefaultClientConnectionOperator(schreg);
    }

    




    protected final void assertStillUp() throws IllegalStateException {
        Asserts.check(!this.isShutDown, "Manager is shut down");
    }

    public final ClientConnectionRequest requestConnection(
            final HttpRoute route,
            final Object state) {

        return new ClientConnectionRequest() {

            public void abortRequest() {
                
            }

            public ManagedClientConnection getConnection(
                    final long timeout, final TimeUnit tunit) {
                return SingleClientConnManager.this.getConnection(
                        route, state);
            }

        };
    }

    







    public ManagedClientConnection getConnection(final HttpRoute route, final Object state) {
        Args.notNull(route, "Route");
        assertStillUp();

        if (log.isDebugEnabled()) {
            log.debug("Get connection for route " + route);
        }

        synchronized (this) {

            Asserts.check(managedConn == null, MISUSE_MESSAGE);

            
            boolean recreate = false;
            boolean shutdown = false;

            
            closeExpiredConnections();

            if (uniquePoolEntry.connection.isOpen()) {
                final RouteTracker tracker = uniquePoolEntry.tracker;
                shutdown = (tracker == null || 
                            !tracker.toRoute().equals(route));
            } else {
                
                
                
                
                
                recreate = true;
            }

            if (shutdown) {
                recreate = true;
                try {
                    uniquePoolEntry.shutdown();
                } catch (final IOException iox) {
                    log.debug("Problem shutting down connection.", iox);
                }
            }

            if (recreate) {
                uniquePoolEntry = new PoolEntry();
            }

            managedConn = new ConnAdapter(uniquePoolEntry, route);

            return managedConn;
        }
    }

    public void releaseConnection(
            final ManagedClientConnection conn,
            final long validDuration, final TimeUnit timeUnit) {
        Args.check(conn instanceof ConnAdapter, "Connection class mismatch, " +
            "connection not obtained from this manager");
        assertStillUp();

        if (log.isDebugEnabled()) {
            log.debug("Releasing connection " + conn);
        }

        final ConnAdapter sca = (ConnAdapter) conn;
        synchronized (sca) {
            if (sca.poolEntry == null)
             {
                return; 
            }
            final ClientConnectionManager manager = sca.getManager();
            Asserts.check(manager == this, "Connection not obtained from this manager");
            try {
                
                if (sca.isOpen() && (this.alwaysShutDown ||
                                     !sca.isMarkedReusable())
                    ) {
                    if (log.isDebugEnabled()) {
                        log.debug
                            ("Released connection open but not reusable.");
                    }

                    
                    
                    
                    sca.shutdown();
                }
            } catch (final IOException iox) {
                if (log.isDebugEnabled()) {
                    log.debug("Exception shutting down released connection.",
                              iox);
                }
            } finally {
                sca.detach();
                synchronized (this) {
                    managedConn = null;
                    lastReleaseTime = System.currentTimeMillis();
                    if(validDuration > 0) {
                        connectionExpiresTime = timeUnit.toMillis(validDuration) + lastReleaseTime;
                    } else {
                        connectionExpiresTime = Long.MAX_VALUE;
                    }
                }
            }
        }
    }

    public void closeExpiredConnections() {
        final long time = connectionExpiresTime;
        if (System.currentTimeMillis() >= time) {
            closeIdleConnections(0, TimeUnit.MILLISECONDS);
        }
    }

    public void closeIdleConnections(final long idletime, final TimeUnit tunit) {
        assertStillUp();

        
        Args.notNull(tunit, "Time unit");

        synchronized (this) {
            if ((managedConn == null) && uniquePoolEntry.connection.isOpen()) {
                final long cutoff =
                    System.currentTimeMillis() - tunit.toMillis(idletime);
                if (lastReleaseTime <= cutoff) {
                    try {
                        uniquePoolEntry.close();
                    } catch (final IOException iox) {
                        
                        log.debug("Problem closing idle connection.", iox);
                    }
                }
            }
        }
    }

    public void shutdown() {
        this.isShutDown = true;
        synchronized (this) {
            try {
                if (uniquePoolEntry != null) {
                    uniquePoolEntry.shutdown();
                }
            } catch (final IOException iox) {
                
                log.debug("Problem while shutting down manager.", iox);
            } finally {
                uniquePoolEntry = null;
                managedConn = null;
            }
        }
    }

    protected void revokeConnection() {
        final ConnAdapter conn = managedConn;
        if (conn == null) {
            return;
        }
        conn.detach();

        synchronized (this) {
            try {
                uniquePoolEntry.shutdown();
            } catch (final IOException iox) {
                
                log.debug("Problem while shutting down connection.", iox);
            }
        }
    }

    


    protected class PoolEntry extends AbstractPoolEntry {

        



        protected PoolEntry() {
            super(SingleClientConnManager.this.connOperator, null);
        }

        


        protected void close() throws IOException {
            shutdownEntry();
            if (connection.isOpen()) {
                connection.close();
            }
        }

        


        protected void shutdown() throws IOException {
            shutdownEntry();
            if (connection.isOpen()) {
                connection.shutdown();
            }
        }

    }

    


    protected class ConnAdapter extends AbstractPooledConnAdapter {

        





        protected ConnAdapter(final PoolEntry entry, final HttpRoute route) {
            super(SingleClientConnManager.this, entry);
            markReusable();
            entry.route = route;
        }

    }

}
