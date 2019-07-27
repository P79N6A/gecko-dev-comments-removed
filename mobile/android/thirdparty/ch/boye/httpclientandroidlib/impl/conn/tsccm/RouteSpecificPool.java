

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.io.IOException;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Queue;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.params.ConnPerRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;
import ch.boye.httpclientandroidlib.util.LangUtils;











@Deprecated
public class RouteSpecificPool {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    
    protected final HttpRoute route; 

    protected final int maxEntries;

    
    protected final ConnPerRoute connPerRoute;

    




    protected final LinkedList<BasicPoolEntry> freeEntries;

    
    protected final Queue<WaitingThread> waitingThreads;

    
    protected int numEntries;

    


    @Deprecated
    public RouteSpecificPool(final HttpRoute route, final int maxEntries) {
        this.route = route;
        this.maxEntries = maxEntries;
        this.connPerRoute = new ConnPerRoute() {
            public int getMaxForRoute(final HttpRoute route) {
                return RouteSpecificPool.this.maxEntries;
            }
        };
        this.freeEntries = new LinkedList<BasicPoolEntry>();
        this.waitingThreads = new LinkedList<WaitingThread>();
        this.numEntries = 0;
    }


    





    public RouteSpecificPool(final HttpRoute route, final ConnPerRoute connPerRoute) {
        this.route = route;
        this.connPerRoute = connPerRoute;
        this.maxEntries = connPerRoute.getMaxForRoute(route);
        this.freeEntries = new LinkedList<BasicPoolEntry>();
        this.waitingThreads = new LinkedList<WaitingThread>();
        this.numEntries = 0;
    }


    




    public final HttpRoute getRoute() {
        return route;
    }


    




    public final int getMaxEntries() {
        return maxEntries;
    }


    







    public boolean isUnused() {
        return (numEntries < 1) && waitingThreads.isEmpty();
    }


    




    public int getCapacity() {
        return connPerRoute.getMaxForRoute(route) - numEntries;
    }


    






    public final int getEntryCount() {
        return numEntries;
    }


    




    public BasicPoolEntry allocEntry(final Object state) {
        if (!freeEntries.isEmpty()) {
            final ListIterator<BasicPoolEntry> it = freeEntries.listIterator(freeEntries.size());
            while (it.hasPrevious()) {
                final BasicPoolEntry entry = it.previous();
                if (entry.getState() == null || LangUtils.equals(state, entry.getState())) {
                    it.remove();
                    return entry;
                }
            }
        }
        if (getCapacity() == 0 && !freeEntries.isEmpty()) {
            final BasicPoolEntry entry = freeEntries.remove();
            entry.shutdownEntry();
            final OperatedClientConnection conn = entry.getConnection();
            try {
                conn.close();
            } catch (final IOException ex) {
                log.debug("I/O error closing connection", ex);
            }
            return entry;
        }
        return null;
    }


    





    public void freeEntry(final BasicPoolEntry entry) {
        if (numEntries < 1) {
            throw new IllegalStateException
                ("No entry created for this pool. " + route);
        }
        if (numEntries <= freeEntries.size()) {
            throw new IllegalStateException
                ("No entry allocated from this pool. " + route);
        }
        freeEntries.add(entry);
    }


    







    public void createdEntry(final BasicPoolEntry entry) {
        Args.check(route.equals(entry.getPlannedRoute()), "Entry not planned for this pool");
        numEntries++;
    }


    









    public boolean deleteEntry(final BasicPoolEntry entry) {

        final boolean found = freeEntries.remove(entry);
        if (found) {
            numEntries--;
        }
        return found;
    }


    





    public void dropEntry() {
        Asserts.check(numEntries > 0, "There is no entry that could be dropped");
        numEntries--;
    }


    







    public void queueThread(final WaitingThread wt) {
        Args.notNull(wt, "Waiting thread");
        this.waitingThreads.add(wt);
    }


    





    public boolean hasThread() {
        return !this.waitingThreads.isEmpty();
    }


    




    public WaitingThread nextThread() {
        return this.waitingThreads.peek();
    }


    




    public void removeThread(final WaitingThread wt) {
        if (wt == null) {
            return;
        }

        this.waitingThreads.remove(wt);
    }


} 
