


























package ch.boye.httpclientandroidlib.impl.conn;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.SchemePortResolver;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class DefaultProxyRoutePlanner extends DefaultRoutePlanner {

    private final HttpHost proxy;

    public DefaultProxyRoutePlanner(final HttpHost proxy, final SchemePortResolver schemePortResolver) {
        super(schemePortResolver);
        this.proxy = Args.notNull(proxy, "Proxy host");
    }

    public DefaultProxyRoutePlanner(final HttpHost proxy) {
        this(proxy, null);
    }

    @Override
    protected HttpHost determineProxy(
        final HttpHost target,
        final HttpRequest request,
        final HttpContext context) throws HttpException {
        return proxy;
    }

}
