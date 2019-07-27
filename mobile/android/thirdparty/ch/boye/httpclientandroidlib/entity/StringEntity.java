


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;
import java.nio.charset.UnsupportedCharsetException;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;







@NotThreadSafe
public class StringEntity extends AbstractHttpEntity implements Cloneable {

    protected final byte[] content;

    











    public StringEntity(final String string, final ContentType contentType) throws UnsupportedCharsetException {
        super();
        Args.notNull(string, "Source string");
        Charset charset = contentType != null ? contentType.getCharset() : null;
        if (charset == null) {
            charset = HTTP.DEF_CONTENT_CHARSET;
        }
        try {
            this.content = string.getBytes(charset.name());
        } catch (final UnsupportedEncodingException ex) {
            
            throw new UnsupportedCharsetException(charset.name());
        }
        if (contentType != null) {
            setContentType(contentType.toString());
        }
    }

    














    @Deprecated
    public StringEntity(
            final String string, final String mimeType, final String charset) throws UnsupportedEncodingException {
        super();
        Args.notNull(string, "Source string");
        final String mt = mimeType != null ? mimeType : HTTP.PLAIN_TEXT_TYPE;
        final String cs = charset != null ? charset :HTTP.DEFAULT_CONTENT_CHARSET;
        this.content = string.getBytes(cs);
        setContentType(mt + HTTP.CHARSET_PARAM + cs);
    }

    











    public StringEntity(final String string, final String charset)
            throws UnsupportedCharsetException {
        this(string, ContentType.create(ContentType.TEXT_PLAIN.getMimeType(), charset));
    }

    











    public StringEntity(final String string, final Charset charset) {
        this(string, ContentType.create(ContentType.TEXT_PLAIN.getMimeType(), charset));
    }

    








    public StringEntity(final String string)
            throws UnsupportedEncodingException {
        this(string, ContentType.DEFAULT_TEXT);
    }

    public boolean isRepeatable() {
        return true;
    }

    public long getContentLength() {
        return this.content.length;
    }

    public InputStream getContent() throws IOException {
        return new ByteArrayInputStream(this.content);
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        Args.notNull(outstream, "Output stream");
        outstream.write(this.content);
        outstream.flush();
    }

    




    public boolean isStreaming() {
        return false;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

} 
