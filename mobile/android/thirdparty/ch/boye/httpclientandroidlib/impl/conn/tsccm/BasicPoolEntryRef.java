

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.util.Args;










@Deprecated
public class BasicPoolEntryRef extends WeakReference<BasicPoolEntry> {

    
    private final HttpRoute route; 


    





    public BasicPoolEntryRef(final BasicPoolEntry entry,
                             final ReferenceQueue<Object> queue) {
        super(entry, queue);
        Args.notNull(entry, "Pool entry");
        route = entry.getPlannedRoute();
    }


    





    public final HttpRoute getRoute() {
        return this.route;
    }

} 

