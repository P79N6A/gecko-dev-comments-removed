


























package ch.boye.httpclientandroidlib.impl.entity;

import java.io.IOException;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.impl.io.ChunkedOutputStream;
import ch.boye.httpclientandroidlib.impl.io.ContentLengthOutputStream;
import ch.boye.httpclientandroidlib.impl.io.IdentityOutputStream;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.util.Args;


















@Immutable 
@Deprecated
public class EntitySerializer {

    private final ContentLengthStrategy lenStrategy;

    public EntitySerializer(final ContentLengthStrategy lenStrategy) {
        super();
        this.lenStrategy = Args.notNull(lenStrategy, "Content length strategy");
    }

    













    protected OutputStream doSerialize(
            final SessionOutputBuffer outbuffer,
            final HttpMessage message) throws HttpException, IOException {
        final long len = this.lenStrategy.determineLength(message);
        if (len == ContentLengthStrategy.CHUNKED) {
            return new ChunkedOutputStream(outbuffer);
        } else if (len == ContentLengthStrategy.IDENTITY) {
            return new IdentityOutputStream(outbuffer);
        } else {
            return new ContentLengthOutputStream(outbuffer, len);
        }
    }

    









    public void serialize(
            final SessionOutputBuffer outbuffer,
            final HttpMessage message,
            final HttpEntity entity) throws HttpException, IOException {
        Args.notNull(outbuffer, "Session output buffer");
        Args.notNull(message, "HTTP message");
        Args.notNull(entity, "HTTP entity");
        final OutputStream outstream = doSerialize(outbuffer, message);
        entity.writeTo(outstream);
        outstream.close();
    }

}
