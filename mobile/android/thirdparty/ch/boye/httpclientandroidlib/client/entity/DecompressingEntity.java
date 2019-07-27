

























package ch.boye.httpclientandroidlib.client.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;
import ch.boye.httpclientandroidlib.util.Args;






abstract class DecompressingEntity extends HttpEntityWrapper {

    


    private static final int BUFFER_SIZE = 1024 * 2;

    



    private InputStream content;

    





    public DecompressingEntity(final HttpEntity wrapped) {
        super(wrapped);
    }

    abstract InputStream decorate(final InputStream wrapped) throws IOException;

    private InputStream getDecompressingStream() throws IOException {
        final InputStream in = wrappedEntity.getContent();
        return new LazyDecompressingInputStream(in, this);
    }

    


    @Override
    public InputStream getContent() throws IOException {
        if (wrappedEntity.isStreaming()) {
            if (content == null) {
                content = getDecompressingStream();
            }
            return content;
        } else {
            return getDecompressingStream();
        }
    }

    


    @Override
    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final InputStream instream = getContent();
        try {
            final byte[] buffer = new byte[BUFFER_SIZE];
            int l;
            while ((l = instream.read(buffer)) != -1) {
                outstream.write(buffer, 0, l);
            }
        } finally {
            instream.close();
        }
    }

}
