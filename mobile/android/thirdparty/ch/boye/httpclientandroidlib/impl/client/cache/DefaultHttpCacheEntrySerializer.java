

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntrySerializationException;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntrySerializer;









@Immutable
public class DefaultHttpCacheEntrySerializer implements HttpCacheEntrySerializer {

    public void writeTo(final HttpCacheEntry cacheEntry, final OutputStream os) throws IOException {
        final ObjectOutputStream oos = new ObjectOutputStream(os);
        try {
            oos.writeObject(cacheEntry);
        } finally {
            oos.close();
        }
    }

    public HttpCacheEntry readFrom(final InputStream is) throws IOException {
        final ObjectInputStream ois = new ObjectInputStream(is);
        try {
            return (HttpCacheEntry) ois.readObject();
        } catch (final ClassNotFoundException ex) {
            throw new HttpCacheEntrySerializationException("Class not found: " + ex.getMessage(), ex);
        } finally {
            ois.close();
        }
    }

}
