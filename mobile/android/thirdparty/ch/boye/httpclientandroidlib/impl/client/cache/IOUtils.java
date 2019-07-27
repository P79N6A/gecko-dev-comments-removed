

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.annotation.Immutable;

@Immutable
class IOUtils {

    static void consume(final HttpEntity entity) throws IOException {
        if (entity == null) {
            return;
        }
        if (entity.isStreaming()) {
            final InputStream instream = entity.getContent();
            if (instream != null) {
                instream.close();
            }
        }
    }

    static void copy(final InputStream in, final OutputStream out) throws IOException {
        final byte[] buf = new byte[2048];
        int len;
        while ((len = in.read(buf)) != -1) {
            out.write(buf, 0, len);
        }
    }

    static void closeSilently(final Closeable closable) {
        try {
            closable.close();
        } catch (final IOException ignore) {
        }
    }

    static void copyAndClose(final InputStream in, final OutputStream out) throws IOException {
        try {
            copy(in, out);
            in.close();
            out.close();
        } catch (final IOException ex) {
            closeSilently(in);
            closeSilently(out);
            
            throw ex;
        }
    }

    static void copyFile(final File in, final File out) throws IOException {
        final RandomAccessFile f1 = new RandomAccessFile(in, "r");
        final RandomAccessFile f2 = new RandomAccessFile(out, "rw");
        try {
            final FileChannel c1 = f1.getChannel();
            final FileChannel c2 = f2.getChannel();
            try {
                c1.transferTo(0, f1.length(), c2);
                c1.close();
                c2.close();
            } catch (final IOException ex) {
                closeSilently(c1);
                closeSilently(c2);
                
                throw ex;
            }
            f1.close();
            f2.close();
        } catch (final IOException ex) {
            closeSilently(f1);
            closeSilently(f2);
            
            throw ex;
        }
    }

}
