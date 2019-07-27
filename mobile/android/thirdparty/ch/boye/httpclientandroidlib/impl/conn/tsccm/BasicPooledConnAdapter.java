

























package ch.boye.httpclientandroidlib.impl.conn.tsccm;

import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.impl.conn.AbstractPoolEntry;
import ch.boye.httpclientandroidlib.impl.conn.AbstractPooledConnAdapter;










@Deprecated
public class BasicPooledConnAdapter extends AbstractPooledConnAdapter {

    





    protected BasicPooledConnAdapter(final ThreadSafeClientConnManager tsccm,
                               final AbstractPoolEntry entry) {
        super(tsccm, entry);
        markReusable();
    }

    @Override
    protected ClientConnectionManager getManager() {
        
        return super.getManager();
    }

    @Override
    protected AbstractPoolEntry getPoolEntry() {
        
        return super.getPoolEntry();
    }

    @Override
    protected void detach() {
        
        super.detach();
    }

}
