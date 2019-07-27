

























package ch.boye.httpclientandroidlib.client.entity;




























import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.zip.GZIPOutputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;







public class GzipCompressingEntity extends HttpEntityWrapper {

    private static final String GZIP_CODEC = "gzip";

    public GzipCompressingEntity(final HttpEntity entity) {
        super(entity);
    }

    @Override
    public Header getContentEncoding() {
        return new BasicHeader(HTTP.CONTENT_ENCODING, GZIP_CODEC);
    }

    @Override
    public long getContentLength() {
        return -1;
    }

    @Override
    public boolean isChunked() {
        
        return true;
    }

    @Override
    public InputStream getContent() throws IOException {
        throw new UnsupportedOperationException();
    }

    @Override
    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        final GZIPOutputStream gzip = new GZIPOutputStream(outstream);
        wrappedEntity.writeTo(gzip);
        
        
        gzip.close();
    }

}
