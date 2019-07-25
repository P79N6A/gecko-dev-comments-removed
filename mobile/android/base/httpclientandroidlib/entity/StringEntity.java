


























package ch.boye.httpclientandroidlib.entity;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;

import ch.boye.httpclientandroidlib.protocol.HTTP;







public class StringEntity extends AbstractHttpEntity implements Cloneable {

    protected final byte[] content;

    









    public StringEntity(final String string, String mimeType, String charset)
            throws UnsupportedEncodingException {
        super();
        if (string == null) {
            throw new IllegalArgumentException("Source string may not be null");
        }
        if (mimeType == null) {
            mimeType = HTTP.PLAIN_TEXT_TYPE;
        }
        if (charset == null) {
            charset = HTTP.DEFAULT_CONTENT_CHARSET;
        }
        this.content = string.getBytes(charset);
        setContentType(mimeType + HTTP.CHARSET_PARAM + charset);
    }

    









    public StringEntity(final String string, String charset)
            throws UnsupportedEncodingException {
        this(string, null, charset);
    }

    










    public StringEntity(final String string)
            throws UnsupportedEncodingException {
        this(string, null);
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
        if (outstream == null) {
            throw new IllegalArgumentException("Output stream may not be null");
        }
        outstream.write(this.content);
        outstream.flush();
    }

    




    public boolean isStreaming() {
        return false;
    }

    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

} 
