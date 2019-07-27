

























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.HttpConnection;
import ch.boye.httpclientandroidlib.config.ConnectionConfig;






public interface HttpConnectionFactory<T, C extends HttpConnection> {

    C create(T route, ConnectionConfig config);

}
