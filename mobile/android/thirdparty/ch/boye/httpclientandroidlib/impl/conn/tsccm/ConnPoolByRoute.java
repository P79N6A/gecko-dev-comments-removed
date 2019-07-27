

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ConnectionPoolTimeoutException;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.params.ConnManagerParams;
import ch.boye.httpclientandroidlib.conn.params.ConnPerRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;


















@Deprecated
public class ConnPoolByRoute extends AbstractConnPool {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final Lock poolLock;

    
    protected final ClientConnectionOperator operator;

    
    protected final ConnPerRoute connPerRoute;

    
    protected final Set<BasicPoolEntry> leasedConnections;

    
    protected final Queue<BasicPoolEntry> freeConnections;

    
    protected final Queue<WaitingThread> waitingThreads;

    
    protected final Map<HttpRoute, RouteSpecificPool> routeToPool;

    private final long connTTL;

    private final TimeUnit connTTLTimeUnit;

    protected volatile boolean shutdown;

    protected volatile int maxTotalConnections;

    protected volatile int numConnections;

    




    public ConnPoolByRoute(
            final ClientConnectionOperator operator,
            final ConnPerRoute connPerRoute,
            final int maxTotalConnections) {
        this(operator, connPerRoute, maxTotalConnections, -1, TimeUnit.MILLISECONDS);
    }

    


    public ConnPoolByRoute(
            final ClientConnectionOperator operator,
            final ConnPerRoute connPerRoute,
            final int maxTotalConnections,
            final long connTTL,
            final TimeUnit connTTLTimeUnit) {
        super();
        Args.notNull(operator, "Connection operator");
        Args.notNull(connPerRoute, "Connections per route");
        this.poolLock = super.poolLock;
        this.leasedConnections = super.leasedConnections;
        this.operator = operator;
        this.connPerRoute = connPerRoute;
        this.maxTotalConnections = maxTotalConnections;
        this.freeConnections = createFreeConnQueue();
        this.waitingThreads  = createWaitingThreadQueue();
        this.routeToPool     = createRouteToPoolMap();
        this.connTTL = connTTL;
        this.connTTLTimeUnit = connTTLTimeUnit;
    }

    protected Lock getLock() {
        return this.poolLock;
    }

    




    @Deprecated
    public ConnPoolByRoute(final ClientConnectionOperator operator, final HttpParams params) {
        this(operator,
                ConnManagerParams.getMaxConnectionsPerRoute(params),
                ConnManagerParams.getMaxTotalConnections(params));
    }

    





    protected Queue<BasicPoolEntry> createFreeConnQueue() {
        return new LinkedList<BasicPoolEntry>();
    }

    





    protected Queue<WaitingThread> createWaitingThreadQueue() {
        return new LinkedList<WaitingThread>();
    }

    





    protected Map<HttpRoute, RouteSpecificPool> createRouteToPoolMap() {
        return new HashMap<HttpRoute, RouteSpecificPool>();
    }


    







    protected RouteSpecificPool newRouteSpecificPool(final HttpRoute route) {
        return new RouteSpecificPool(route, this.connPerRoute);
    }


    








    protected WaitingThread newWaitingThread(final Condition cond,
                                             final RouteSpecificPool rospl) {
        return new WaitingThread(cond, rospl);
    }

    private void closeConnection(final BasicPoolEntry entry) {
        final OperatedClientConnection conn = entry.getConnection();
        if (conn != null) {
            try {
                conn.close();
            } catch (final IOException ex) {
                log.debug("I/O error closing connection", ex);
            }
        }
    }

    








    protected RouteSpecificPool getRoutePool(final HttpRoute route,
                                             final boolean create) {
        RouteSpecificPool rospl = null;
        poolLock.lock();
        try {

            rospl = routeToPool.get(route);
            if ((rospl == null) && create) {
                
                rospl = newRouteSpecificPool(route);
                routeToPool.put(route, rospl);
            }

        } finally {
            poolLock.unlock();
        }

        return rospl;
    }

    public int getConnectionsInPool(final HttpRoute route) {
        poolLock.lock();
        try {
            
            final RouteSpecificPool rospl = getRoutePool(route, false);
            return (rospl != null) ? rospl.getEntryCount() : 0;

        } finally {
            poolLock.unlock();
        }
    }

    public int getConnectionsInPool() {
        poolLock.lock();
        try {
            return numConnections;
        } finally {
            poolLock.unlock();
        }
    }

    @Override
    public PoolEntryRequest requestPoolEntry(
            final HttpRoute route,
            final Object state) {

        final WaitingThreadAborter aborter = new WaitingThreadAborter();

        return new PoolEntryRequest() {

            public void abortRequest() {
                poolLock.lock();
                try {
                    aborter.abort();
                } finally {
                    poolLock.unlock();
                }
            }

            public BasicPoolEntry getPoolEntry(
                    final long timeout,
                    final TimeUnit tunit)
                        throws InterruptedException, ConnectionPoolTimeoutException {
                return getEntryBlocking(route, state, timeout, tunit, aborter);
            }

        };
    }

    

















    protected BasicPoolEntry getEntryBlocking(
                                   final HttpRoute route, final Object state,
                                   final long timeout, final TimeUnit tunit,
                                   final WaitingThreadAborter aborter)
        throws ConnectionPoolTimeoutException, InterruptedException {

        Date deadline = null;
        if (timeout > 0) {
            deadline = new Date
                (System.currentTimeMillis() + tunit.toMillis(timeout));
        }

        BasicPoolEntry entry = null;
        poolLock.lock();
        try {

            RouteSpecificPool rospl = getRoutePool(route, true);
            WaitingThread waitingThread = null;

            while (entry == null) {
                Asserts.check(!shutdown, "Connection pool shut down");

                if (log.isDebugEnabled()) {
                    log.debug("[" + route + "] total kept alive: " + freeConnections.size() +
                            ", total issued: " + leasedConnections.size() +
                            ", total allocated: " + numConnections + " out of " + maxTotalConnections);
                }

                
                
                
                
                

                entry = getFreeEntry(rospl, state);
                if (entry != null) {
                    break;
                }

                final boolean hasCapacity = rospl.getCapacity() > 0;

                if (log.isDebugEnabled()) {
                    log.debug("Available capacity: " + rospl.getCapacity()
                            + " out of " + rospl.getMaxEntries()
                            + " [" + route + "][" + state + "]");
                }

                if (hasCapacity && numConnections < maxTotalConnections) {

                    entry = createEntry(rospl, operator);

                } else if (hasCapacity && !freeConnections.isEmpty()) {

                    deleteLeastUsedEntry();
                    
                    
                    rospl = getRoutePool(route, true);
                    entry = createEntry(rospl, operator);

                } else {

                    if (log.isDebugEnabled()) {
                        log.debug("Need to wait for connection" +
                                " [" + route + "][" + state + "]");
                    }

                    if (waitingThread == null) {
                        waitingThread =
                            newWaitingThread(poolLock.newCondition(), rospl);
                        aborter.setWaitingThread(waitingThread);
                    }

                    boolean success = false;
                    try {
                        rospl.queueThread(waitingThread);
                        waitingThreads.add(waitingThread);
                        success = waitingThread.await(deadline);

                    } finally {
                        
                        
                        
                        
                        rospl.removeThread(waitingThread);
                        waitingThreads.remove(waitingThread);
                    }

                    
                    if (!success && (deadline != null) &&
                        (deadline.getTime() <= System.currentTimeMillis())) {
                        throw new ConnectionPoolTimeoutException
                            ("Timeout waiting for connection from pool");
                    }
                }
            } 

        } finally {
            poolLock.unlock();
        }
        return entry;
    }

    @Override
    public void freeEntry(final BasicPoolEntry entry, final boolean reusable, final long validDuration, final TimeUnit timeUnit) {

        final HttpRoute route = entry.getPlannedRoute();
        if (log.isDebugEnabled()) {
            log.debug("Releasing connection" +
                    " [" + route + "][" + entry.getState() + "]");
        }

        poolLock.lock();
        try {
            if (shutdown) {
                
                
                closeConnection(entry);
                return;
            }

            
            leasedConnections.remove(entry);

            final RouteSpecificPool rospl = getRoutePool(route, true);

            if (reusable && rospl.getCapacity() >= 0) {
                if (log.isDebugEnabled()) {
                    final String s;
                    if (validDuration > 0) {
                        s = "for " + validDuration + " " + timeUnit;
                    } else {
                        s = "indefinitely";
                    }
                    log.debug("Pooling connection" +
                            " [" + route + "][" + entry.getState() + "]; keep alive " + s);
                }
                rospl.freeEntry(entry);
                entry.updateExpiry(validDuration, timeUnit);
                freeConnections.add(entry);
            } else {
                closeConnection(entry);
                rospl.dropEntry();
                numConnections--;
            }

            notifyWaitingThread(rospl);

        } finally {
            poolLock.unlock();
        }
    }

    







    protected BasicPoolEntry getFreeEntry(final RouteSpecificPool rospl, final Object state) {

        BasicPoolEntry entry = null;
        poolLock.lock();
        try {
            boolean done = false;
            while(!done) {

                entry = rospl.allocEntry(state);

                if (entry != null) {
                    if (log.isDebugEnabled()) {
                        log.debug("Getting free connection"
                                + " [" + rospl.getRoute() + "][" + state + "]");

                    }
                    freeConnections.remove(entry);
                    if (entry.isExpired(System.currentTimeMillis())) {
                        
                        
                        if (log.isDebugEnabled()) {
                            log.debug("Closing expired free connection"
                                    + " [" + rospl.getRoute() + "][" + state + "]");
                        }
                        closeConnection(entry);
                        
                        
                        
                        rospl.dropEntry();
                        numConnections--;
                    } else {
                        leasedConnections.add(entry);
                        done = true;
                    }

                } else {
                    done = true;
                    if (log.isDebugEnabled()) {
                        log.debug("No free connections"
                                + " [" + rospl.getRoute() + "][" + state + "]");
                    }
                }
            }
        } finally {
            poolLock.unlock();
        }
        return entry;
    }


    









    protected BasicPoolEntry createEntry(final RouteSpecificPool rospl,
                                         final ClientConnectionOperator op) {

        if (log.isDebugEnabled()) {
            log.debug("Creating new connection [" + rospl.getRoute() + "]");
        }

        
        final BasicPoolEntry entry = new BasicPoolEntry(op, rospl.getRoute(), connTTL, connTTLTimeUnit);

        poolLock.lock();
        try {
            rospl.createdEntry(entry);
            numConnections++;
            leasedConnections.add(entry);
        } finally {
            poolLock.unlock();
        }

        return entry;
    }


    










    protected void deleteEntry(final BasicPoolEntry entry) {

        final HttpRoute route = entry.getPlannedRoute();

        if (log.isDebugEnabled()) {
            log.debug("Deleting connection"
                    + " [" + route + "][" + entry.getState() + "]");
        }

        poolLock.lock();
        try {

            closeConnection(entry);

            final RouteSpecificPool rospl = getRoutePool(route, true);
            rospl.deleteEntry(entry);
            numConnections--;
            if (rospl.isUnused()) {
                routeToPool.remove(route);
            }

        } finally {
            poolLock.unlock();
        }
    }


    



    protected void deleteLeastUsedEntry() {
        poolLock.lock();
        try {

            final BasicPoolEntry entry = freeConnections.remove();

            if (entry != null) {
                deleteEntry(entry);
            } else if (log.isDebugEnabled()) {
                log.debug("No free connection to delete");
            }

        } finally {
            poolLock.unlock();
        }
    }

    @Override
    protected void handleLostEntry(final HttpRoute route) {

        poolLock.lock();
        try {

            final RouteSpecificPool rospl = getRoutePool(route, true);
            rospl.dropEntry();
            if (rospl.isUnused()) {
                routeToPool.remove(route);
            }

            numConnections--;
            notifyWaitingThread(rospl);

        } finally {
            poolLock.unlock();
        }
    }

    







    protected void notifyWaitingThread(final RouteSpecificPool rospl) {

        
        
        
        
        
        WaitingThread waitingThread = null;

        poolLock.lock();
        try {

            if ((rospl != null) && rospl.hasThread()) {
                if (log.isDebugEnabled()) {
                    log.debug("Notifying thread waiting on pool" +
                            " [" + rospl.getRoute() + "]");
                }
                waitingThread = rospl.nextThread();
            } else if (!waitingThreads.isEmpty()) {
                if (log.isDebugEnabled()) {
                    log.debug("Notifying thread waiting on any pool");
                }
                waitingThread = waitingThreads.remove();
            } else if (log.isDebugEnabled()) {
                log.debug("Notifying no-one, there are no waiting threads");
            }

            if (waitingThread != null) {
                waitingThread.wakeup();
            }

        } finally {
            poolLock.unlock();
        }
    }


    @Override
    public void deleteClosedConnections() {
        poolLock.lock();
        try {
            final Iterator<BasicPoolEntry>  iter = freeConnections.iterator();
            while (iter.hasNext()) {
                final BasicPoolEntry entry = iter.next();
                if (!entry.getConnection().isOpen()) {
                    iter.remove();
                    deleteEntry(entry);
                }
            }
        } finally {
            poolLock.unlock();
        }
    }

    






    @Override
    public void closeIdleConnections(final long idletime, final TimeUnit tunit) {
        Args.notNull(tunit, "Time unit");
        final long t = idletime > 0 ? idletime : 0;
        if (log.isDebugEnabled()) {
            log.debug("Closing connections idle longer than "  + t + " " + tunit);
        }
        
        final long deadline = System.currentTimeMillis() - tunit.toMillis(t);
        poolLock.lock();
        try {
            final Iterator<BasicPoolEntry>  iter = freeConnections.iterator();
            while (iter.hasNext()) {
                final BasicPoolEntry entry = iter.next();
                if (entry.getUpdated() <= deadline) {
                    if (log.isDebugEnabled()) {
                        log.debug("Closing connection last used @ " + new Date(entry.getUpdated()));
                    }
                    iter.remove();
                    deleteEntry(entry);
                }
            }
        } finally {
            poolLock.unlock();
        }
    }

    @Override
    public void closeExpiredConnections() {
        log.debug("Closing expired connections");
        final long now = System.currentTimeMillis();

        poolLock.lock();
        try {
            final Iterator<BasicPoolEntry>  iter = freeConnections.iterator();
            while (iter.hasNext()) {
                final BasicPoolEntry entry = iter.next();
                if (entry.isExpired(now)) {
                    if (log.isDebugEnabled()) {
                        log.debug("Closing connection expired @ " + new Date(entry.getExpiry()));
                    }
                    iter.remove();
                    deleteEntry(entry);
                }
            }
        } finally {
            poolLock.unlock();
        }
    }

    @Override
    public void shutdown() {
        poolLock.lock();
        try {
            if (shutdown) {
                return;
            }
            shutdown = true;

            
            final Iterator<BasicPoolEntry> iter1 = leasedConnections.iterator();
            while (iter1.hasNext()) {
                final BasicPoolEntry entry = iter1.next();
                iter1.remove();
                closeConnection(entry);
            }

            
            final Iterator<BasicPoolEntry> iter2 = freeConnections.iterator();
            while (iter2.hasNext()) {
                final BasicPoolEntry entry = iter2.next();
                iter2.remove();

                if (log.isDebugEnabled()) {
                    log.debug("Closing connection"
                            + " [" + entry.getPlannedRoute() + "][" + entry.getState() + "]");
                }
                closeConnection(entry);
            }

            
            final Iterator<WaitingThread> iwth = waitingThreads.iterator();
            while (iwth.hasNext()) {
                final WaitingThread waiter = iwth.next();
                iwth.remove();
                waiter.wakeup();
            }

            routeToPool.clear();

        } finally {
            poolLock.unlock();
        }
    }

    


    public void setMaxTotalConnections(final int max) {
        poolLock.lock();
        try {
            maxTotalConnections = max;
        } finally {
            poolLock.unlock();
        }
    }


    


    public int getMaxTotalConnections() {
        return maxTotalConnections;
    }

}

