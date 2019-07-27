

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.InputLimit;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.client.cache.ResourceFactory;






@Immutable
public class HeapResourceFactory implements ResourceFactory {

    public Resource generate(
            final String requestId,
            final InputStream instream,
            final InputLimit limit) throws IOException {
        final ByteArrayOutputStream outstream = new ByteArrayOutputStream();
        final byte[] buf = new byte[2048];
        long total = 0;
        int l;
        while ((l = instream.read(buf)) != -1) {
            outstream.write(buf, 0, l);
            total += l;
            if (limit != null && total > limit.getValue()) {
                limit.reached();
                break;
            }
        }
        return createResource(outstream.toByteArray());
    }

    public Resource copy(
            final String requestId,
            final Resource resource) throws IOException {
        byte[] body;
        if (resource instanceof HeapResource) {
            body = ((HeapResource) resource).getByteArray();
        } else {
            final ByteArrayOutputStream outstream = new ByteArrayOutputStream();
            IOUtils.copyAndClose(resource.getInputStream(), outstream);
            body = outstream.toByteArray();
        }
        return createResource(body);
    }

    Resource createResource(final byte[] buf) {
        return new HeapResource(buf);
    }

}
