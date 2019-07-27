

























package ch.boye.httpclientandroidlib.impl.pool;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpClientConnection;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.pool.PoolEntry;









@ThreadSafe
public class BasicPoolEntry extends PoolEntry<HttpHost, HttpClientConnection> {

    public BasicPoolEntry(final String id, final HttpHost route, final HttpClientConnection conn) {
        super(id, route, conn);
    }

    @Override
    public void close() {
        try {
            this.getConnection().close();
        } catch (final IOException ignore) {
        }
    }

    @Override
    public boolean isClosed() {
        return !this.getConnection().isOpen();
    }

}
