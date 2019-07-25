


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.RouteTracker;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.params.HttpParams;















@ThreadSafe
public class SingleClientConnManager implements ClientConnectionManager {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    
    public final static String MISUSE_MESSAGE =
    "Invalid use of SingleClientConnManager: connection still allocated.\n" +
    "Make sure to release the connection before allocating another one.";

    
    protected final SchemeRegistry schemeRegistry;

    
    protected final ClientConnectionOperator connOperator;

    
    protected final boolean alwaysShutDown;

    
    @GuardedBy("this")
    protected PoolEntry uniquePoolEntry;

    
    @GuardedBy("this")
    protected ConnAdapter managedConn;

    
    @GuardedBy("this")
    protected long lastReleaseTime;

    
    @GuardedBy("this")
    protected long connectionExpiresTime;

    
    protected volatile boolean isShutDown;

    







    @Deprecated
    public SingleClientConnManager(HttpParams params,
                                   SchemeRegistry schreg) {
        this(schreg);
    }
    




    public SingleClientConnManager(final SchemeRegistry schreg) {
        if (schreg == null) {
            throw new IllegalArgumentException
                ("Scheme registry must not be null.");
        }
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
        createConnectionOperator(SchemeRegistry schreg) {
        return new DefaultClientConnectionOperator(schreg);
    }

    




    protected final void assertStillUp() throws IllegalStateException {
        if (this.isShutDown)
            throw new IllegalStateException("Manager is shut down.");
    }

    public final ClientConnectionRequest requestConnection(
            final HttpRoute route,
            final Object state) {

        return new ClientConnectionRequest() {

            public void abortRequest() {
                
            }

            public ManagedClientConnection getConnection(
                    long timeout, TimeUnit tunit) {
                return SingleClientConnManager.this.getConnection(
                        route, state);
            }

        };
    }

    







    public synchronized ManagedClientConnection getConnection(HttpRoute route, Object state) {
        if (route == null) {
            throw new IllegalArgumentException("Route may not be null.");
        }
        assertStillUp();

        if (log.isDebugEnabled()) {
            log.debug("Get connection for route " + route);
        }

        if (managedConn != null)
            throw new IllegalStateException(MISUSE_MESSAGE);

        
        boolean recreate = false;
        boolean shutdown = false;

        
        closeExpiredConnections();

        if (uniquePoolEntry.connection.isOpen()) {
            RouteTracker tracker = uniquePoolEntry.tracker;
            shutdown = (tracker == null || 
                        !tracker.toRoute().equals(route));
        } else {
            
            
            
            
            
            recreate = true;
        }

        if (shutdown) {
            recreate = true;
            try {
                uniquePoolEntry.shutdown();
            } catch (IOException iox) {
                log.debug("Problem shutting down connection.", iox);
            }
        }

        if (recreate)
            uniquePoolEntry = new PoolEntry();

        managedConn = new ConnAdapter(uniquePoolEntry, route);

        return managedConn;
    }

    public synchronized void releaseConnection(
            ManagedClientConnection conn,
            long validDuration, TimeUnit timeUnit) {
        assertStillUp();

        if (!(conn instanceof ConnAdapter)) {
            throw new IllegalArgumentException
                ("Connection class mismatch, " +
                 "connection not obtained from this manager.");
        }

        if (log.isDebugEnabled()) {
            log.debug("Releasing connection " + conn);
        }

        ConnAdapter sca = (ConnAdapter) conn;
        if (sca.poolEntry == null)
            return; 
        ClientConnectionManager manager = sca.getManager();
        if (manager != null && manager != this) {
            throw new IllegalArgumentException
                ("Connection not obtained from this manager.");
        }

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
        } catch (IOException iox) {
            if (log.isDebugEnabled())
                log.debug("Exception shutting down released connection.",
                          iox);
        } finally {
            sca.detach();
            managedConn = null;
            lastReleaseTime = System.currentTimeMillis();
            if(validDuration > 0)
                connectionExpiresTime = timeUnit.toMillis(validDuration) + lastReleaseTime;
            else
                connectionExpiresTime = Long.MAX_VALUE;
        }
    }

    public synchronized void closeExpiredConnections() {
        if(System.currentTimeMillis() >= connectionExpiresTime) {
            closeIdleConnections(0, TimeUnit.MILLISECONDS);
        }
    }

    public synchronized void closeIdleConnections(long idletime, TimeUnit tunit) {
        assertStillUp();

        
        if (tunit == null) {
            throw new IllegalArgumentException("Time unit must not be null.");
        }

        if ((managedConn == null) && uniquePoolEntry.connection.isOpen()) {
            final long cutoff =
                System.currentTimeMillis() - tunit.toMillis(idletime);
            if (lastReleaseTime <= cutoff) {
                try {
                    uniquePoolEntry.close();
                } catch (IOException iox) {
                    
                    log.debug("Problem closing idle connection.", iox);
                }
            }
        }
    }

    public synchronized void shutdown() {

        this.isShutDown = true;

        if (managedConn != null)
            managedConn.detach();

        try {
            if (uniquePoolEntry != null) 
                uniquePoolEntry.shutdown();
        } catch (IOException iox) {
            
            log.debug("Problem while shutting down manager.", iox);
        } finally {
            uniquePoolEntry = null;
        }
    }

    


    @Deprecated
    protected synchronized void revokeConnection() {
        if (managedConn == null)
            return;
        managedConn.detach();
        try {
            uniquePoolEntry.shutdown();
        } catch (IOException iox) {
            
            log.debug("Problem while shutting down connection.", iox);
        }
    }

    


    protected class PoolEntry extends AbstractPoolEntry {

        



        protected PoolEntry() {
            super(SingleClientConnManager.this.connOperator, null);
        }

        


        protected void close() throws IOException {
            shutdownEntry();
            if (connection.isOpen())
                connection.close();
        }

        


        protected void shutdown() throws IOException {
            shutdownEntry();
            if (connection.isOpen())
                connection.shutdown();
        }

    }

    


    protected class ConnAdapter extends AbstractPooledConnAdapter {

        





        protected ConnAdapter(PoolEntry entry, HttpRoute route) {
            super(SingleClientConnManager.this, entry);
            markReusable();
            entry.route = route;
        }

    }

}
