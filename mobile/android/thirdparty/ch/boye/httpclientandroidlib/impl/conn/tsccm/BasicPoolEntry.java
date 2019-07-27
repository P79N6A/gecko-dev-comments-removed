

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import java.lang.ref.ReferenceQueue;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.impl.conn.AbstractPoolEntry;
import ch.boye.httpclientandroidlib.util.Args;








@Deprecated
public class BasicPoolEntry extends AbstractPoolEntry {

    private final long created;

    private long updated;
    private final long validUntil;
    private long expiry;

    public BasicPoolEntry(final ClientConnectionOperator op,
                          final HttpRoute route,
                          final ReferenceQueue<Object> queue) {
        super(op, route);
        Args.notNull(route, "HTTP route");
        this.created = System.currentTimeMillis();
        this.validUntil = Long.MAX_VALUE;
        this.expiry = this.validUntil;
    }

    





    public BasicPoolEntry(final ClientConnectionOperator op,
                          final HttpRoute route) {
        this(op, route, -1, TimeUnit.MILLISECONDS);
    }

    









    public BasicPoolEntry(final ClientConnectionOperator op,
                          final HttpRoute route, final long connTTL, final TimeUnit timeunit) {
        super(op, route);
        Args.notNull(route, "HTTP route");
        this.created = System.currentTimeMillis();
        if (connTTL > 0) {
            this.validUntil = this.created + timeunit.toMillis(connTTL);
        } else {
            this.validUntil = Long.MAX_VALUE;
        }
        this.expiry = this.validUntil;
    }

    protected final OperatedClientConnection getConnection() {
        return super.connection;
    }

    protected final HttpRoute getPlannedRoute() {
        return super.route;
    }

    protected final BasicPoolEntryRef getWeakRef() {
        return null;
    }

    @Override
    protected void shutdownEntry() {
        super.shutdownEntry();
    }

    


    public long getCreated() {
        return this.created;
    }

    


    public long getUpdated() {
        return this.updated;
    }

    


    public long getExpiry() {
        return this.expiry;
    }

    public long getValidUntil() {
        return this.validUntil;
    }

    


    public void updateExpiry(final long time, final TimeUnit timeunit) {
        this.updated = System.currentTimeMillis();
        final long newExpiry;
        if (time > 0) {
            newExpiry = this.updated + timeunit.toMillis(time);
        } else {
            newExpiry = Long.MAX_VALUE;
        }
        this.expiry = Math.min(validUntil, newExpiry);
    }

    


    public boolean isExpired(final long now) {
        return now >= this.expiry;
    }

}


