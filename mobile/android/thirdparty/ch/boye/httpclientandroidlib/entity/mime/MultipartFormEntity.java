


























package ch.boye.httpclientandroidlib.entity.mime;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.message.BasicHeader;
import ch.boye.httpclientandroidlib.protocol.HTTP;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class MultipartFormEntity implements HttpEntity {

    private final AbstractMultipartForm multipart;
    private final Header contentType;
    private final long contentLength;

    MultipartFormEntity(
            final AbstractMultipartForm multipart,
            final String contentType,
            final long contentLength) {
        super();
        this.multipart = multipart;
        this.contentType = new BasicHeader(HTTP.CONTENT_TYPE, contentType);
        this.contentLength = contentLength;
    }

    AbstractMultipartForm getMultipart() {
        return this.multipart;
    }

    public boolean isRepeatable() {
        return this.contentLength != -1;
    }

    public boolean isChunked() {
        return !isRepeatable();
    }

    public boolean isStreaming() {
        return !isRepeatable();
    }

    public long getContentLength() {
        return this.contentLength;
    }

    public Header getContentType() {
        return this.contentType;
    }

    public Header getContentEncoding() {
        return null;
    }

    public void consumeContent()
        throws IOException, UnsupportedOperationException{
        if (isStreaming()) {
            throw new UnsupportedOperationException(
                    "Streaming entity does not implement #consumeContent()");
        }
    }

    public InputStream getContent() throws IOException {
        throw new UnsupportedOperationException(
                    "Multipart form entity does not implement #getContent()");
    }

    public void writeTo(final OutputStream outstream) throws IOException {
        this.multipart.writeTo(outstream);
    }

}
