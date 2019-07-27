

























package ch.boye.httpclientandroidlib.client.entity;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

import java.io.IOException;
import java.io.InputStream;




@NotThreadSafe
class LazyDecompressingInputStream extends InputStream {

    private final InputStream wrappedStream;

    private final DecompressingEntity decompressingEntity;

    private InputStream wrapperStream;

    public LazyDecompressingInputStream(
            final InputStream wrappedStream,
            final DecompressingEntity decompressingEntity) {
        this.wrappedStream = wrappedStream;
        this.decompressingEntity = decompressingEntity;
    }

    private void initWrapper() throws IOException {
        if (wrapperStream == null) {
            wrapperStream = decompressingEntity.decorate(wrappedStream);
        }
    }

    @Override
    public int read() throws IOException {
        initWrapper();
        return wrapperStream.read();
    }

    @Override
    public int read(final byte[] b) throws IOException {
        initWrapper();
        return wrapperStream.read(b);
    }

    @Override
    public int read(final byte[] b, final int off, final int len) throws IOException {
        initWrapper();
        return wrapperStream.read(b, off, len);
    }

    @Override
    public long skip(final long n) throws IOException {
        initWrapper();
        return wrapperStream.skip(n);
    }

    @Override
    public boolean markSupported() {
        return false;
    }

    @Override
    public int available() throws IOException {
        initWrapper();
        return wrapperStream.available();
    }

    @Override
    public void close() throws IOException {
        try {
            if (wrapperStream != null) {
                wrapperStream.close();
            }
        } finally {
            wrappedStream.close();
        }
    }

}
