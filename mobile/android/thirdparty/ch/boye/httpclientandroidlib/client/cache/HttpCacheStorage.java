

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.IOException;









public interface HttpCacheStorage {

    





    void putEntry(String key, HttpCacheEntry entry) throws IOException;

    







    HttpCacheEntry getEntry(String key) throws IOException;

    





    void removeEntry(String key) throws IOException;

    










    void updateEntry(
            String key, HttpCacheUpdateCallback callback) throws IOException, HttpCacheUpdateException;

}
