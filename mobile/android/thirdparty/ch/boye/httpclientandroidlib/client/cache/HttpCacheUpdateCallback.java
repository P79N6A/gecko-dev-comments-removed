

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.IOException;







public interface HttpCacheUpdateCallback {

    










    HttpCacheEntry update(HttpCacheEntry existing) throws IOException;

}
