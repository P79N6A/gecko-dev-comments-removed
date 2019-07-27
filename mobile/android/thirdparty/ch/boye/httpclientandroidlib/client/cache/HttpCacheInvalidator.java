

























package ch.boye.httpclientandroidlib.client.cache;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;







public interface HttpCacheInvalidator {

    








    void flushInvalidatedCacheEntries(HttpHost host, HttpRequest req);

    



    void flushInvalidatedCacheEntries(HttpHost host, HttpRequest request, HttpResponse response);

}
