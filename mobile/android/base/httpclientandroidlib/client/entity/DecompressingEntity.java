
























package ch.boye.httpclientandroidlib.client.entity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;






abstract class DecompressingEntity extends HttpEntityWrapper {

    


    private static final int BUFFER_SIZE = 1024 * 2;

    



    private InputStream content;

    





    public DecompressingEntity(final HttpEntity wrapped) {
        super(wrapped);
    }

    abstract InputStream getDecompressingInputStream(final InputStream wrapped) throws IOException;

    


    @Override
    public InputStream getContent() throws IOException {
        if (wrappedEntity.isStreaming()) {
            if (content == null) {
                content = getDecompressingInputStream(wrappedEntity.getContent());
            }
            return content;
        } else {
            return getDecompressingInputStream(wrappedEntity.getContent());
        }
    }

    


    @Override
    public void writeTo(OutputStream outstream) throws IOException {
        if (outstream == null) {
            throw new IllegalArgumentException("Output stream may not be null");
        }
        InputStream instream = getContent();
        try {
            byte[] buffer = new byte[BUFFER_SIZE];

            int l;

            while ((l = instream.read(buffer)) != -1) {
                outstream.write(buffer, 0, l);
            }
        } finally {
            instream.close();
        }
    }

}
