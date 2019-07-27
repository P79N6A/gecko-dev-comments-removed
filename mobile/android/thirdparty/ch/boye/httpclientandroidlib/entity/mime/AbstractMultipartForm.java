


























package ch.boye.httpclientandroidlib.entity.mime;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.util.List;

import ch.boye.httpclientandroidlib.entity.mime.content.ContentBody;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.ByteArrayBuffer;








abstract class AbstractMultipartForm {

    private static ByteArrayBuffer encode(
            final Charset charset, final String string) {
        final ByteBuffer encoded = charset.encode(CharBuffer.wrap(string));
        final ByteArrayBuffer bab = new ByteArrayBuffer(encoded.remaining());
        bab.append(encoded.array(), encoded.position(), encoded.remaining());
        return bab;
    }

    private static void writeBytes(
            final ByteArrayBuffer b, final OutputStream out) throws IOException {
        out.write(b.buffer(), 0, b.length());
    }

    private static void writeBytes(
            final String s, final Charset charset, final OutputStream out) throws IOException {
        final ByteArrayBuffer b = encode(charset, s);
        writeBytes(b, out);
    }

    private static void writeBytes(
            final String s, final OutputStream out) throws IOException {
        final ByteArrayBuffer b = encode(MIME.DEFAULT_CHARSET, s);
        writeBytes(b, out);
    }

    protected static void writeField(
            final MinimalField field, final OutputStream out) throws IOException {
        writeBytes(field.getName(), out);
        writeBytes(FIELD_SEP, out);
        writeBytes(field.getBody(), out);
        writeBytes(CR_LF, out);
    }

    protected static void writeField(
            final MinimalField field, final Charset charset, final OutputStream out) throws IOException {
        writeBytes(field.getName(), charset, out);
        writeBytes(FIELD_SEP, out);
        writeBytes(field.getBody(), charset, out);
        writeBytes(CR_LF, out);
    }

    private static final ByteArrayBuffer FIELD_SEP = encode(MIME.DEFAULT_CHARSET, ": ");
    private static final ByteArrayBuffer CR_LF = encode(MIME.DEFAULT_CHARSET, "\r\n");
    private static final ByteArrayBuffer TWO_DASHES = encode(MIME.DEFAULT_CHARSET, "--");

    private final String subType;
    protected final Charset charset;
    private final String boundary;

    







    public AbstractMultipartForm(final String subType, final Charset charset, final String boundary) {
        super();
        Args.notNull(subType, "Multipart subtype");
        Args.notNull(boundary, "Multipart boundary");
        this.subType = subType;
        this.charset = charset != null ? charset : MIME.DEFAULT_CHARSET;
        this.boundary = boundary;
    }

    public AbstractMultipartForm(final String subType, final String boundary) {
        this(subType, null, boundary);
    }

    public String getSubType() {
        return this.subType;
    }

    public Charset getCharset() {
        return this.charset;
    }

    public abstract List<FormBodyPart> getBodyParts();

    public String getBoundary() {
        return this.boundary;
    }

    void doWriteTo(
        final OutputStream out,
        final boolean writeContent) throws IOException {

        final ByteArrayBuffer boundary = encode(this.charset, getBoundary());
        for (final FormBodyPart part: getBodyParts()) {
            writeBytes(TWO_DASHES, out);
            writeBytes(boundary, out);
            writeBytes(CR_LF, out);

            formatMultipartHeader(part, out);

            writeBytes(CR_LF, out);

            if (writeContent) {
                part.getBody().writeTo(out);
            }
            writeBytes(CR_LF, out);
        }
        writeBytes(TWO_DASHES, out);
        writeBytes(boundary, out);
        writeBytes(TWO_DASHES, out);
        writeBytes(CR_LF, out);
    }

    


    protected abstract void formatMultipartHeader(
        final FormBodyPart part,
        final OutputStream out) throws IOException;

    




    public void writeTo(final OutputStream out) throws IOException {
        doWriteTo(out, true);
    }

    












    public long getTotalLength() {
        long contentLen = 0;
        for (final FormBodyPart part: getBodyParts()) {
            final ContentBody body = part.getBody();
            final long len = body.getContentLength();
            if (len >= 0) {
                contentLen += len;
            } else {
                return -1;
            }
        }
        final ByteArrayOutputStream out = new ByteArrayOutputStream();
        try {
            doWriteTo(out, false);
            final byte[] extra = out.toByteArray();
            return contentLen + extra.length;
        } catch (final IOException ex) {
            
            return -1;
        }
    }

}
