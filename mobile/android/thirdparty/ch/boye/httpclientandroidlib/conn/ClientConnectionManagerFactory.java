


























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.params.HttpParams;








@Deprecated
public interface ClientConnectionManagerFactory {

    ClientConnectionManager newInstance(
            HttpParams params,
            SchemeRegistry schemeRegistry);

}
