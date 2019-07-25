
























package ch.boye.httpclientandroidlib.client.entity;

import java.io.IOException;
import java.io.InputStream;
import java.util.zip.GZIPInputStream;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;






public class GzipDecompressingEntity extends DecompressingEntity {

    






    public GzipDecompressingEntity(final HttpEntity entity) {
        super(entity);
    }

    @Override
    InputStream getDecompressingInputStream(final InputStream wrapped) throws IOException {
        return new GZIPInputStream(wrapped);
    }

    


    @Override
    public Header getContentEncoding() {

        
        return null;
    }

    


    @Override
    public long getContentLength() {

        
        return -1;
    }

}
