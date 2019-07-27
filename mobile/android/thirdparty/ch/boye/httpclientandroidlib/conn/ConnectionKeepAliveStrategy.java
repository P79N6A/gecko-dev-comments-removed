

























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.protocol.HttpContext;











public interface ConnectionKeepAliveStrategy {

    



















    long getKeepAliveDuration(HttpResponse response, HttpContext context);

}
