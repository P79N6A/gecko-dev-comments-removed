

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;
import java.util.Date;
import java.util.Map;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;




interface HttpCache {

    





    void flushCacheEntriesFor(HttpHost host, HttpRequest request)
        throws IOException;

    





    void flushInvalidatedCacheEntriesFor(HttpHost host, HttpRequest request)
        throws IOException;

    





    void flushInvalidatedCacheEntriesFor(HttpHost host, HttpRequest request,
            HttpResponse response);

    






    HttpCacheEntry getCacheEntry(HttpHost host, HttpRequest request)
        throws IOException;

    







    Map<String,Variant> getVariantCacheEntriesWithEtags(HttpHost host, HttpRequest request)
        throws IOException;

    









    HttpResponse cacheAndReturnResponse(
            HttpHost host, HttpRequest request, HttpResponse originResponse,
            Date requestSent, Date responseReceived)
        throws IOException;

    









    CloseableHttpResponse cacheAndReturnResponse(HttpHost host,
            HttpRequest request, CloseableHttpResponse originResponse,
            Date requestSent, Date responseReceived)
        throws IOException;

    










    HttpCacheEntry updateCacheEntry(
            HttpHost target, HttpRequest request, HttpCacheEntry stale, HttpResponse originResponse,
            Date requestSent, Date responseReceived)
        throws IOException;

    












    HttpCacheEntry updateVariantCacheEntry(HttpHost target, HttpRequest request,
            HttpCacheEntry stale, HttpResponse originResponse, Date requestSent,
            Date responseReceived, String cacheKey)
        throws IOException;

    







    void reuseVariantEntryFor(HttpHost target, final HttpRequest req,
            final Variant variant) throws IOException;
}
