


























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.params.HttpParams;







public interface ClientConnectionManagerFactory {

    ClientConnectionManager newInstance(
            HttpParams params,
            SchemeRegistry schemeRegistry);

}
