

























package ch.boye.httpclientandroidlib.impl.client;

import java.util.HashMap;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.client.AuthCache;
import ch.boye.httpclientandroidlib.conn.SchemePortResolver;
import ch.boye.httpclientandroidlib.conn.UnsupportedSchemeException;
import ch.boye.httpclientandroidlib.impl.conn.DefaultSchemePortResolver;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class BasicAuthCache implements AuthCache {

    private final HashMap<HttpHost, AuthScheme> map;
    private final SchemePortResolver schemePortResolver;

    




    public BasicAuthCache(final SchemePortResolver schemePortResolver) {
        super();
        this.map = new HashMap<HttpHost, AuthScheme>();
        this.schemePortResolver = schemePortResolver != null ? schemePortResolver :
            DefaultSchemePortResolver.INSTANCE;
    }

    public BasicAuthCache() {
        this(null);
    }

    protected HttpHost getKey(final HttpHost host) {
        if (host.getPort() <= 0) {
            final int port;
            try {
                port = schemePortResolver.resolve(host);
            } catch (final UnsupportedSchemeException ignore) {
                return host;
            }
            return new HttpHost(host.getHostName(), port, host.getSchemeName());
        } else {
            return host;
        }
    }

    public void put(final HttpHost host, final AuthScheme authScheme) {
        Args.notNull(host, "HTTP host");
        this.map.put(getKey(host), authScheme);
    }

    public AuthScheme get(final HttpHost host) {
        Args.notNull(host, "HTTP host");
        return this.map.get(getKey(host));
    }

    public void remove(final HttpHost host) {
        Args.notNull(host, "HTTP host");
        this.map.remove(getKey(host));
    }

    public void clear() {
        this.map.clear();
    }

    @Override
    public String toString() {
        return this.map.toString();
    }

}
