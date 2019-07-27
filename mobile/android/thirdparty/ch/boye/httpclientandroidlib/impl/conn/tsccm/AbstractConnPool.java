

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.io.IOException;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.impl.conn.IdleConnectionHandler;
import ch.boye.httpclientandroidlib.util.Args;












@Deprecated
public abstract class AbstractConnPool {

    public HttpClientAndroidLog log;

    


    protected final Lock poolLock;

    
    @GuardedBy("poolLock")
    protected Set<BasicPoolEntry> leasedConnections;

    
    @GuardedBy("poolLock")
    protected int numConnections;

    
    protected volatile boolean isShutDown;

    protected Set<BasicPoolEntryRef> issuedConnections;

    protected ReferenceQueue<Object> refQueue;

    protected IdleConnectionHandler idleConnHandler;

    


    protected AbstractConnPool() {
        super();
        this.log = new HttpClientAndroidLog(getClass());
        this.leasedConnections = new HashSet<BasicPoolEntry>();
        this.idleConnHandler = new IdleConnectionHandler();
        this.poolLock = new ReentrantLock();
    }

    public void enableConnectionGC()
        throws IllegalStateException {
    }

    














    public final
        BasicPoolEntry getEntry(
                final HttpRoute route,
                final Object state,
                final long timeout,
                final TimeUnit tunit)
                    throws ConnectionPoolTimeoutException, InterruptedException {
        return requestPoolEntry(route, state).getPoolEntry(timeout, tunit);
    }

    



    public abstract PoolEntryRequest requestPoolEntry(HttpRoute route, Object state);


    











    public abstract void freeEntry(BasicPoolEntry entry, boolean reusable, long validDuration, TimeUnit timeUnit)
        ;

    public void handleReference(final Reference<?> ref) {
    }

    protected abstract void handleLostEntry(HttpRoute route);

    






    public void closeIdleConnections(final long idletime, final TimeUnit tunit) {

        
        Args.notNull(tunit, "Time unit");

        poolLock.lock();
        try {
            idleConnHandler.closeIdleConnections(tunit.toMillis(idletime));
        } finally {
            poolLock.unlock();
        }
    }

    public void closeExpiredConnections() {
        poolLock.lock();
        try {
            idleConnHandler.closeExpiredConnections();
        } finally {
            poolLock.unlock();
        }
    }


    


    public abstract void deleteClosedConnections();

    



    public void shutdown() {

        poolLock.lock();
        try {

            if (isShutDown) {
                return;
            }

            
            final Iterator<BasicPoolEntry> iter = leasedConnections.iterator();
            while (iter.hasNext()) {
                final BasicPoolEntry entry = iter.next();
                iter.remove();
                closeConnection(entry.getConnection());
            }
            idleConnHandler.removeAll();

            isShutDown = true;

        } finally {
            poolLock.unlock();
        }
    }


    




    protected void closeConnection(final OperatedClientConnection conn) {
        if (conn != null) {
            try {
                conn.close();
            } catch (final IOException ex) {
                log.debug("I/O error closing connection", ex);
            }
        }
    }

} 

