

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.params.ConnPerRouteBean;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.impl.conn.DefaultClientConnectionOperator;
import ch.boye.httpclientandroidlib.impl.conn.SchemeRegistryFactory;



















@ThreadSafe
public class ThreadSafeClientConnManager implements ClientConnectionManager {

    public HttpClientAndroidLog log;

    
    protected final SchemeRegistry schemeRegistry; 

    @Deprecated
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
            long connTTL, TimeUnit connTTLTimeUnit) {
        super();
        if (schreg == null) {
            throw new IllegalArgumentException("Scheme registry may not be null");
        }
        this.log = new HttpClientAndroidLog(getClass());
        this.schemeRegistry = schreg;
        this.connPerRoute = new ConnPerRouteBean();
        this.connOperator = createConnectionOperator(schreg);
        this.pool = createConnectionPool(connTTL, connTTLTimeUnit) ;
        this.connectionPool = this.pool;
    }

    







    @Deprecated
    public ThreadSafeClientConnManager(HttpParams params,
                                       SchemeRegistry schreg) {
        if (schreg == null) {
            throw new IllegalArgumentException("Scheme registry may not be null");
        }
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

    






    protected ConnPoolByRoute createConnectionPool(long connTTL, TimeUnit connTTLTimeUnit) {
        return new ConnPoolByRoute(connOperator, connPerRoute, 20, connTTL, connTTLTimeUnit);
    }

    











    protected ClientConnectionOperator
        createConnectionOperator(SchemeRegistry schreg) {

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
                    long timeout, TimeUnit tunit) throws InterruptedException,
                    ConnectionPoolTimeoutException {
                if (route == null) {
                    throw new IllegalArgumentException("Route may not be null.");
                }

                if (log.isDebugEnabled()) {
                    log.debug("Get connection: " + route + ", timeout = " + timeout);
                }

                BasicPoolEntry entry = poolRequest.getPoolEntry(timeout, tunit);
                return new BasicPooledConnAdapter(ThreadSafeClientConnManager.this, entry);
            }

        };

    }

    public void releaseConnection(ManagedClientConnection conn, long validDuration, TimeUnit timeUnit) {

        if (!(conn instanceof BasicPooledConnAdapter)) {
            throw new IllegalArgumentException
                ("Connection class mismatch, " +
                 "connection not obtained from this manager.");
        }
        BasicPooledConnAdapter hca = (BasicPooledConnAdapter) conn;
        if ((hca.getPoolEntry() != null) && (hca.getManager() != this)) {
            throw new IllegalArgumentException
                ("Connection not obtained from this manager.");
        }
        synchronized (hca) {
            BasicPoolEntry entry = (BasicPoolEntry) hca.getPoolEntry();
            if (entry == null) {
                return;
            }
            try {
                
                if (hca.isOpen() && !hca.isMarkedReusable()) {
                    
                    
                    

                    
                    
                    
                    
                    hca.shutdown();
                }
            } catch (IOException iox) {
                if (log.isDebugEnabled())
                    log.debug("Exception shutting down released connection.",
                              iox);
            } finally {
                boolean reusable = hca.isMarkedReusable();
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

    public void closeIdleConnections(long idleTimeout, TimeUnit tunit) {
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

    


    public void setMaxTotal(int max) {
        pool.setMaxTotalConnections(max);
    }

    


    public int getDefaultMaxPerRoute() {
        return connPerRoute.getDefaultMaxPerRoute();
    }

    


    public void setDefaultMaxPerRoute(int max) {
        connPerRoute.setDefaultMaxPerRoute(max);
    }

    


    public int getMaxForRoute(final HttpRoute route) {
        return connPerRoute.getMaxForRoute(route);
    }

    


    public void setMaxForRoute(final HttpRoute route, int max) {
        connPerRoute.setMaxForRoute(route, max);
    }

}

