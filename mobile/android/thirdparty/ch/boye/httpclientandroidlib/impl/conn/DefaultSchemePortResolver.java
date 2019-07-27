

























package ch.boye.httpclientandroidlib.impl.conn;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.SchemePortResolver;
import ch.boye.httpclientandroidlib.conn.UnsupportedSchemeException;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class DefaultSchemePortResolver implements SchemePortResolver {

    public static final DefaultSchemePortResolver INSTANCE = new DefaultSchemePortResolver();

    public int resolve(final HttpHost host) throws UnsupportedSchemeException {
        Args.notNull(host, "HTTP host");
        final int port = host.getPort();
        if (port > 0) {
            return port;
        }
        final String name = host.getSchemeName();
        if (name.equalsIgnoreCase("http")) {
            return 80;
        } else if (name.equalsIgnoreCase("https")) {
            return 443;
        } else {
            throw new UnsupportedSchemeException(name + " protocol is not supported");
        }
    }

}
