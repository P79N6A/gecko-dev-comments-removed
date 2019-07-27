

























package ch.boye.httpclientandroidlib.client.cache;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;




@NotThreadSafe
public class HttpCacheContext extends HttpClientContext {

    




    public static final String CACHE_RESPONSE_STATUS = "http.cache.response.status";

    public static HttpCacheContext adapt(final HttpContext context) {
        if (context instanceof HttpCacheContext) {
            return (HttpCacheContext) context;
        } else {
            return new HttpCacheContext(context);
        }
    }

    public static HttpCacheContext create() {
        return new HttpCacheContext(new BasicHttpContext());
    }

    public HttpCacheContext(final HttpContext context) {
        super(context);
    }

    public HttpCacheContext() {
        super();
    }

    public CacheResponseStatus getCacheResponseStatus() {
        return getAttribute(CACHE_RESPONSE_STATUS, CacheResponseStatus.class);
    }

}
