

























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;
import java.util.Locale;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.entity.DeflateDecompressingEntity;
import ch.boye.httpclientandroidlib.client.entity.GzipDecompressingEntity;
import ch.boye.httpclientandroidlib.protocol.HttpContext;










@Immutable
public class ResponseContentEncoding implements HttpResponseInterceptor {

    public static final String UNCOMPRESSED = "http.client.response.uncompressed";

    













    public void process(
            final HttpResponse response,
            final HttpContext context) throws HttpException, IOException {
        final HttpEntity entity = response.getEntity();

        
        
        if (entity != null && entity.getContentLength() != 0) {
            final Header ceheader = entity.getContentEncoding();
            if (ceheader != null) {
                final HeaderElement[] codecs = ceheader.getElements();
                boolean uncompressed = false;
                for (final HeaderElement codec : codecs) {
                    final String codecname = codec.getName().toLowerCase(Locale.ENGLISH);
                    if ("gzip".equals(codecname) || "x-gzip".equals(codecname)) {
                        response.setEntity(new GzipDecompressingEntity(response.getEntity()));
                        uncompressed = true;
                        break;
                    } else if ("deflate".equals(codecname)) {
                        response.setEntity(new DeflateDecompressingEntity(response.getEntity()));
                        uncompressed = true;
                        break;
                    } else if ("identity".equals(codecname)) {

                        
                        return;
                    } else {
                        throw new HttpException("Unsupported Content-Coding: " + codec.getName());
                    }
                }
                if (uncompressed) {
                    response.removeHeaders("Content-Length");
                    response.removeHeaders("Content-Encoding");
                    response.removeHeaders("Content-MD5");
                }
            }
        }
    }

}
