

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;















@Deprecated
public abstract class AbstractPooledConnAdapter extends AbstractClientConnAdapter {

    
    protected volatile AbstractPoolEntry poolEntry;

    





    protected AbstractPooledConnAdapter(final ClientConnectionManager manager,
                                        final AbstractPoolEntry entry) {
        super(manager, entry.connection);
        this.poolEntry = entry;
    }

    public String getId() {
        return null;
    }

    






    @Deprecated
    protected AbstractPoolEntry getPoolEntry() {
        return this.poolEntry;
    }

    







    protected void assertValid(final AbstractPoolEntry entry) {
        if (isReleased() || entry == null) {
            throw new ConnectionShutdownException();
        }
    }

    


    @Deprecated
    protected final void assertAttached() {
        if (poolEntry == null) {
            throw new ConnectionShutdownException();
        }
    }

    



    @Override
    protected synchronized void detach() {
        poolEntry = null;
        super.detach();
    }

    public HttpRoute getRoute() {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        return (entry.tracker == null) ? null : entry.tracker.toRoute();
    }

    public void open(final HttpRoute route,
                     final HttpContext context, final HttpParams params)
        throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        entry.open(route, context, params);
    }

    public void tunnelTarget(final boolean secure, final HttpParams params)
        throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        entry.tunnelTarget(secure, params);
    }

    public void tunnelProxy(final HttpHost next, final boolean secure, final HttpParams params)
        throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        entry.tunnelProxy(next, secure, params);
    }

    public void layerProtocol(final HttpContext context, final HttpParams params)
        throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        entry.layerProtocol(context, params);
    }

    public void close() throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        if (entry != null) {
            entry.shutdownEntry();
        }

        final OperatedClientConnection conn = getWrappedConnection();
        if (conn != null) {
            conn.close();
        }
    }

    public void shutdown() throws IOException {
        final AbstractPoolEntry entry = getPoolEntry();
        if (entry != null) {
            entry.shutdownEntry();
        }

        final OperatedClientConnection conn = getWrappedConnection();
        if (conn != null) {
            conn.shutdown();
        }
    }

    public Object getState() {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        return entry.getState();
    }

    public void setState(final Object state) {
        final AbstractPoolEntry entry = getPoolEntry();
        assertValid(entry);
        entry.setState(state);
    }

}
