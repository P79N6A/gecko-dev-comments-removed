

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;


import java.lang.ref.WeakReference;
import java.lang.ref.ReferenceQueue;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;










@Immutable
public class BasicPoolEntryRef extends WeakReference<BasicPoolEntry> {

    
    private final HttpRoute route; 


    





    public BasicPoolEntryRef(BasicPoolEntry entry,
                             ReferenceQueue<Object> queue) {
        super(entry, queue);
        if (entry == null) {
            throw new IllegalArgumentException
                ("Pool entry must not be null.");
        }
        route = entry.getPlannedRoute();
    }


    





    public final HttpRoute getRoute() {
        return this.route;
    }

} 

