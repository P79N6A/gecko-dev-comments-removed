


























package ch.boye.httpclientandroidlib.impl.client;

import java.net.ProxySelector;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.impl.DefaultConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.NoConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.impl.conn.PoolingClientConnectionManager;
import ch.boye.httpclientandroidlib.impl.conn.ProxySelectorRoutePlanner;
import ch.boye.httpclientandroidlib.impl.conn.SchemeRegistryFactory;
import ch.boye.httpclientandroidlib.params.HttpParams;

































































@ThreadSafe
@Deprecated
public class SystemDefaultHttpClient extends DefaultHttpClient {

    public SystemDefaultHttpClient(final HttpParams params) {
        super(null, params);
    }

    public SystemDefaultHttpClient() {
        super(null, null);
    }

    @Override
    protected ClientConnectionManager createClientConnectionManager() {
        final PoolingClientConnectionManager connmgr = new PoolingClientConnectionManager(
                SchemeRegistryFactory.createSystemDefault());
        String s = System.getProperty("http.keepAlive", "true");
        if ("true".equalsIgnoreCase(s)) {
            s = System.getProperty("http.maxConnections", "5");
            final int max = Integer.parseInt(s);
            connmgr.setDefaultMaxPerRoute(max);
            connmgr.setMaxTotal(2 * max);
        }
        return connmgr;
    }

    @Override
    protected HttpRoutePlanner createHttpRoutePlanner() {
        return new ProxySelectorRoutePlanner(getConnectionManager().getSchemeRegistry(),
                ProxySelector.getDefault());
    }

    @Override
    protected ConnectionReuseStrategy createConnectionReuseStrategy() {
        final String s = System.getProperty("http.keepAlive", "true");
        if ("true".equalsIgnoreCase(s)) {
            return new DefaultConnectionReuseStrategy();
        } else {
            return new NoConnectionReuseStrategy();
        }
    }

}
