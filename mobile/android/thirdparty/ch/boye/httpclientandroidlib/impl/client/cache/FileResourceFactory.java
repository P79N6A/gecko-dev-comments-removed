

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.InputLimit;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.client.cache.ResourceFactory;






@Immutable
public class FileResourceFactory implements ResourceFactory {

    private final File cacheDir;
    private final BasicIdGenerator idgen;

    public FileResourceFactory(final File cacheDir) {
        super();
        this.cacheDir = cacheDir;
        this.idgen = new BasicIdGenerator();
    }

    private File generateUniqueCacheFile(final String requestId) {
        final StringBuilder buffer = new StringBuilder();
        this.idgen.generate(buffer);
        buffer.append('.');
        final int len = Math.min(requestId.length(), 100);
        for (int i = 0; i < len; i++) {
            final char ch = requestId.charAt(i);
            if (Character.isLetterOrDigit(ch) || ch == '.') {
                buffer.append(ch);
            } else {
                buffer.append('-');
            }
        }
        return new File(this.cacheDir, buffer.toString());
    }

    public Resource generate(
            final String requestId,
            final InputStream instream,
            final InputLimit limit) throws IOException {
        final File file = generateUniqueCacheFile(requestId);
        final FileOutputStream outstream = new FileOutputStream(file);
        try {
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
        } finally {
            outstream.close();
        }
        return new FileResource(file);
    }

    public Resource copy(
            final String requestId,
            final Resource resource) throws IOException {
        final File file = generateUniqueCacheFile(requestId);

        if (resource instanceof FileResource) {
            final File src = ((FileResource) resource).getFile();
            IOUtils.copyFile(src, file);
        } else {
            final FileOutputStream out = new FileOutputStream(file);
            IOUtils.copyAndClose(resource.getInputStream(), out);
        }
        return new FileResource(file);
    }

}
