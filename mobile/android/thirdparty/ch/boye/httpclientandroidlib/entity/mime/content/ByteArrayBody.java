

























package ch.boye.httpclientandroidlib.entity.mime.content;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.entity.ContentType;
import ch.boye.httpclientandroidlib.entity.mime.MIME;
import ch.boye.httpclientandroidlib.util.Args;








public class ByteArrayBody extends AbstractContentBody {

    


    private final byte[] data;

    


    private final String filename;

    









    @Deprecated
    public ByteArrayBody(final byte[] data, final String mimeType, final String filename) {
        this(data, ContentType.create(mimeType), filename);
    }

    


    public ByteArrayBody(final byte[] data, final ContentType contentType, final String filename) {
        super(contentType);
        Args.notNull(data, "byte[]");
        this.data = data;
        this.filename = filename;
    }

    





    public ByteArrayBody(final byte[] data, final String filename) {
        this(data, "application/octet-stream", filename);
    }

    public String getFilename() {
        return filename;
    }

    public void writeTo(final OutputStream out) throws IOException {
        out.write(data);
    }

    @Override
    public String getCharset() {
        return null;
    }

    public String getTransferEncoding() {
        return MIME.ENC_BINARY;
    }

    public long getContentLength() {
        return data.length;
    }

}
