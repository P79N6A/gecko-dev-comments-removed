

























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HeaderElementIterator;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.ConnectionKeepAliveStrategy;
import ch.boye.httpclientandroidlib.message.BasicHeaderElementIterator;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;










@Immutable
public class DefaultConnectionKeepAliveStrategy implements ConnectionKeepAliveStrategy {

    public static final DefaultConnectionKeepAliveStrategy INSTANCE = new DefaultConnectionKeepAliveStrategy();

    public long getKeepAliveDuration(final HttpResponse response, final HttpContext context) {
        Args.notNull(response, "HTTP response");
        final HeaderElementIterator it = new BasicHeaderElementIterator(
                response.headerIterator(HTTP.CONN_KEEP_ALIVE));
        while (it.hasNext()) {
            final HeaderElement he = it.nextElement();
            final String param = he.getName();
            final String value = he.getValue();
            if (value != null && param.equalsIgnoreCase("timeout")) {
                try {
                    return Long.parseLong(value) * 1000;
                } catch(final NumberFormatException ignore) {
                }
            }
        }
        return -1;
    }

}
