

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.auth.AuthScheme;








public interface AuthCache {

    void put(HttpHost host, AuthScheme authScheme);

    AuthScheme get(HttpHost host);

    void remove(HttpHost host);

    void clear();

}
