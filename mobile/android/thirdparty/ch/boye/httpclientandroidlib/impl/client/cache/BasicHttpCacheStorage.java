

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheStorage;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheUpdateCallback;











@ThreadSafe
public class BasicHttpCacheStorage implements HttpCacheStorage {

    private final CacheMap entries;

    public BasicHttpCacheStorage(final CacheConfig config) {
        super();
        this.entries = new CacheMap(config.getMaxCacheEntries());
    }

    







    public synchronized void putEntry(final String url, final HttpCacheEntry entry) throws IOException {
        entries.put(url, entry);
    }

    






    public synchronized HttpCacheEntry getEntry(final String url) throws IOException {
        return entries.get(url);
    }

    





    public synchronized void removeEntry(final String url) throws IOException {
        entries.remove(url);
    }

    public synchronized void updateEntry(
            final String url,
            final HttpCacheUpdateCallback callback) throws IOException {
        final HttpCacheEntry existingEntry = entries.get(url);
        entries.put(url, callback.update(existingEntry));
    }

}
