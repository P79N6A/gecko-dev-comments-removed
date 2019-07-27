


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.util.Args;







public class EntityTemplate extends AbstractHttpEntity {

    private final ContentProducer contentproducer;

    public EntityTemplate(final ContentProducer contentproducer) {
        super();
        this.contentproducer = Args.notNull(contentproducer, "Content producer");
    }

    public long getContentLength() {
        return -1;
    }

    public InputStream getContent() throws IOException {
        final ByteArrayOutputStream buf = new ByteArrayOutputStream();
        writeTo(buf);
        return new ByteArrayInputStream(buf.toByteArray());
    }

    public boolean isRepeatable() {
        return true;
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        this.contentproducer.writeTo(outstream);
    }

    public boolean isStreaming() {
        return false;
    }

}
