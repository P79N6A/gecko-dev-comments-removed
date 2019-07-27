

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.Resource;






@Immutable
public class HeapResource implements Resource {

    private static final long serialVersionUID = -2078599905620463394L;

    private final byte[] b;

    public HeapResource(final byte[] b) {
        super();
        this.b = b;
    }

    byte[] getByteArray() {
        return this.b;
    }

    public InputStream getInputStream() {
        return new ByteArrayInputStream(this.b);
    }

    public long length() {
        return this.b.length;
    }

    public void dispose() {
    }

}
