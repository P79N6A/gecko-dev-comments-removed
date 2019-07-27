


























package ch.boye.httpclientandroidlib.impl.entity;

import java.io.IOException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.BasicHttpEntity;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.impl.io.ChunkedInputStream;
import ch.boye.httpclientandroidlib.impl.io.ContentLengthInputStream;
import ch.boye.httpclientandroidlib.impl.io.IdentityInputStream;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;



















@Immutable 
@Deprecated
public class EntityDeserializer {

    private final ContentLengthStrategy lenStrategy;

    public EntityDeserializer(final ContentLengthStrategy lenStrategy) {
        super();
        this.lenStrategy = Args.notNull(lenStrategy, "Content length strategy");
    }

    














    protected BasicHttpEntity doDeserialize(
            final SessionInputBuffer inbuffer,
            final HttpMessage message) throws HttpException, IOException {
        final BasicHttpEntity entity = new BasicHttpEntity();

        final long len = this.lenStrategy.determineLength(message);
        if (len == ContentLengthStrategy.CHUNKED) {
            entity.setChunked(true);
            entity.setContentLength(-1);
            entity.setContent(new ChunkedInputStream(inbuffer));
        } else if (len == ContentLengthStrategy.IDENTITY) {
            entity.setChunked(false);
            entity.setContentLength(-1);
            entity.setContent(new IdentityInputStream(inbuffer));
        } else {
            entity.setChunked(false);
            entity.setContentLength(len);
            entity.setContent(new ContentLengthInputStream(inbuffer, len));
        }

        final Header contentTypeHeader = message.getFirstHeader(HTTP.CONTENT_TYPE);
        if (contentTypeHeader != null) {
            entity.setContentType(contentTypeHeader);
        }
        final Header contentEncodingHeader = message.getFirstHeader(HTTP.CONTENT_ENCODING);
        if (contentEncodingHeader != null) {
            entity.setContentEncoding(contentEncodingHeader);
        }
        return entity;
    }

    













    public HttpEntity deserialize(
            final SessionInputBuffer inbuffer,
            final HttpMessage message) throws HttpException, IOException {
        Args.notNull(inbuffer, "Session input buffer");
        Args.notNull(message, "HTTP message");
        return doDeserialize(inbuffer, message);
    }

}
