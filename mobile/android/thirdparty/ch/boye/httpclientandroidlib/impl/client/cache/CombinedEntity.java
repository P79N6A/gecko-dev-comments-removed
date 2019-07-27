

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.SequenceInputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.entity.AbstractHttpEntity;
import ch.boye.httpclientandroidlib.util.Args;

@NotThreadSafe
class CombinedEntity extends AbstractHttpEntity {

    private final Resource resource;
    private final InputStream combinedStream;

    CombinedEntity(final Resource resource, final InputStream instream) throws IOException {
        super();
        this.resource = resource;
        this.combinedStream = new SequenceInputStream(
                new ResourceStream(resource.getInputStream()), instream);
    }

    public long getContentLength() {
        return -1;
    }

    public boolean isRepeatable() {
        return false;
    }

    public boolean isStreaming() {
        return true;
    }

    public InputStream getContent() throws IOException, IllegalStateException {
        return this.combinedStream;
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final InputStream instream = getContent();
        try {
            int l;
            final byte[] tmp = new byte[2048];
            while ((l = instream.read(tmp)) != -1) {
                outstream.write(tmp, 0, l);
            }
        } finally {
            instream.close();
        }
    }

    private void dispose() {
        this.resource.dispose();
    }

    class ResourceStream extends FilterInputStream {

        protected ResourceStream(final InputStream in) {
            super(in);
        }

        @Override
        public void close() throws IOException {
            try {
                super.close();
            } finally {
                dispose();
            }
        }

    }

}
