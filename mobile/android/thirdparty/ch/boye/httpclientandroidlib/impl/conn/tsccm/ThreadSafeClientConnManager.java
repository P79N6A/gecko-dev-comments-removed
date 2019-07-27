

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.params.ConnPerRouteBean;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.impl.conn.DefaultClientConnectionOperator;
import ch.boye.httpclientandroidlib.impl.conn.SchemeRegistryFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;





















@ThreadSafe
@Deprecated
public class ThreadSafeClientConnManager implements ClientConnectionManager {

    public HttpClientAndroidLog log;

    
    protected final SchemeRegistry schemeRegistry; 

    protected final AbstractConnPool connectionPool;

    
    protected final ConnPoolByRoute pool;

    
    protected final ClientConnectionOperator connOperator; 

    protected final ConnPerRouteBean connPerRoute;

    




    public ThreadSafeClientConnManager(final SchemeRegistry schreg) {
        this(schreg, -1, TimeUnit.MILLISECONDS);
    }

    


    public ThreadSafeClientConnManager() {
        this(SchemeRegistryFactory.createDefault());
    }

    








    public ThreadSafeClientConnManager(final SchemeRegistry schreg,
            final long connTTL, final TimeUnit connTTLTimeUnit) {
        this(schreg, connTTL, connTTLTimeUnit, new ConnPerRouteBean());
    }

    











    public ThreadSafeClientConnManager(final SchemeRegistry schreg,
            final long connTTL, final TimeUnit connTTLTimeUnit, final ConnPerRouteBean connPerRoute) {
        super();
        Args.notNull(schreg, "Scheme registry");
        this.log = new HttpClientAndroidLog(getClass());
        this.schemeRegistry = schreg;
        this.connPerRoute = connPerRoute;
        this.connOperator = createConnectionOperator(schreg);
        this.pool = createConnectionPool(connTTL, connTTLTimeUnit) ;
        this.connectionPool = this.pool;
    }

    







    @Deprecated
    public ThreadSafeClientConnManager(final HttpParams params,
                                       final SchemeRegistry schreg) {
        Args.notNull(schreg, "Scheme registry");
        this.log = new HttpClientAndroidLog(getClass());
        this.schemeRegistry = schreg;
        this.connPerRoute = new ConnPerRouteBean();
        this.connOperator = createConnectionOperator(schreg);
        this.pool = (ConnPoolByRoute) createConnectionPool(params) ;
        this.connectionPool = this.pool;
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            shutdown();
        } finally {
            super.finalize();
        }
    }

    






    @Deprecated
    protected AbstractConnPool createConnectionPool(final HttpParams params) {
        return new ConnPoolByRoute(connOperator, params);
    }

    






    protected ConnPoolByRoute createConnectionPool(final long connTTL, final TimeUnit connTTLTimeUnit) {
        return new ConnPoolByRoute(connOperator, connPerRoute, 20, connTTL, connTTLTimeUnit);
    }

    











    protected ClientConnectionOperator
        createConnectionOperator(final SchemeRegistry schreg) {

        return new DefaultClientConnectionOperator(schreg);
    }

    public SchemeRegistry getSchemeRegistry() {
        return this.schemeRegistry;
    }

    public ClientConnectionRequest requestConnection(
            final HttpRoute route,
            final Object state) {

        final PoolEntryRequest poolRequest = pool.requestPoolEntry(
                route, state);

        return new ClientConnectionRequest() {

            public void abortRequest() {
                poolRequest.abortRequest();
            }

            public ManagedClientConnection getConnection(
                    final long timeout, final TimeUnit tunit) throws InterruptedException,
                    ConnectionPoolTimeoutException {
                Args.notNull(route, "Route");

                if (log.isDebugEnabled()) {
                    log.debug("Get connection: " + route + ", timeout = " + timeout);
                }

                final BasicPoolEntry entry = poolRequest.getPoolEntry(timeout, tunit);
                return new BasicPooledConnAdapter(ThreadSafeClientConnManager.this, entry);
            }

        };

    }

    public void releaseConnection(final ManagedClientConnection conn, final long validDuration, final TimeUnit timeUnit) {
        Args.check(conn instanceof BasicPooledConnAdapter, "Connection class mismatch, " +
                "connection not obtained from this manager");
        final BasicPooledConnAdapter hca = (BasicPooledConnAdapter) conn;
        if (hca.getPoolEntry() != null) {
            Asserts.check(hca.getManager() == this, "Connection not obtained from this manager");
        }
        synchronized (hca) {
            final BasicPoolEntry entry = (BasicPoolEntry) hca.getPoolEntry();
            if (entry == null) {
                return;
            }
            try {
                
                if (hca.isOpen() && !hca.isMarkedReusable()) {
                    
                    
                    

                    
                    
                    
                    
                    hca.shutdown();
                }
            } catch (final IOException iox) {
                if (log.isDebugEnabled()) {
                    log.debug("Exception shutting down released connection.",
                              iox);
                }
            } finally {
                final boolean reusable = hca.isMarkedReusable();
                if (log.isDebugEnabled()) {
                    if (reusable) {
                        log.debug("Released connection is reusable.");
                    } else {
                        log.debug("Released connection is not reusable.");
                    }
                }
                hca.detach();
                pool.freeEntry(entry, reusable, validDuration, timeUnit);
            }
        }
    }

    public void shutdown() {
        log.debug("Shutting down");
        pool.shutdown();
    }

    









    public int getConnectionsInPool(final HttpRoute route) {
        return pool.getConnectionsInPool(route);
    }

    







    public int getConnectionsInPool() {
        return pool.getConnectionsInPool();
    }

    public void closeIdleConnections(final long idleTimeout, final TimeUnit tunit) {
        if (log.isDebugEnabled()) {
            log.debug("Closing connections idle longer than " + idleTimeout + " " + tunit);
        }
        pool.closeIdleConnections(idleTimeout, tunit);
    }

    public void closeExpiredConnections() {
        log.debug("Closing expired connections");
        pool.closeExpiredConnections();
    }

    


    public int getMaxTotal() {
        return pool.getMaxTotalConnections();
    }

    


    public void setMaxTotal(final int max) {
        pool.setMaxTotalConnections(max);
    }

    


    public int getDefaultMaxPerRoute() {
        return connPerRoute.getDefaultMaxPerRoute();
    }

    


    public void setDefaultMaxPerRoute(final int max) {
        connPerRoute.setDefaultMaxPerRoute(max);
    }

    


    public int getMaxForRoute(final HttpRoute route) {
        return connPerRoute.getMaxForRoute(route);
    }

    


    public void setMaxForRoute(final HttpRoute route, final int max) {
        connPerRoute.setMaxForRoute(route, max);
    }

}

