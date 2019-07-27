

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;






public interface HttpCacheEntrySerializer {

    




    void writeTo(HttpCacheEntry entry, OutputStream os) throws IOException;

    




    HttpCacheEntry readFrom(InputStream is) throws IOException;

}
