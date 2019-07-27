


























package ch.boye.httpclientandroidlib.impl.entity;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;











@Immutable
public class StrictContentLengthStrategy implements ContentLengthStrategy {

    public static final StrictContentLengthStrategy INSTANCE = new StrictContentLengthStrategy();

    private final int implicitLen;

    







    public StrictContentLengthStrategy(final int implicitLen) {
        super();
        this.implicitLen = implicitLen;
    }

    



    public StrictContentLengthStrategy() {
        this(IDENTITY);
    }

    public long determineLength(final HttpMessage message) throws HttpException {
        Args.notNull(message, "HTTP message");
        
        
        
        final Header transferEncodingHeader = message.getFirstHeader(HTTP.TRANSFER_ENCODING);
        if (transferEncodingHeader != null) {
            final String s = transferEncodingHeader.getValue();
            if (HTTP.CHUNK_CODING.equalsIgnoreCase(s)) {
                if (message.getProtocolVersion().lessEquals(HttpVersion.HTTP_1_0)) {
                    throw new ProtocolException(
                            "Chunked transfer encoding not allowed for " +
                            message.getProtocolVersion());
                }
                return CHUNKED;
            } else if (HTTP.IDENTITY_CODING.equalsIgnoreCase(s)) {
                return IDENTITY;
            } else {
                throw new ProtocolException(
                        "Unsupported transfer encoding: " + s);
            }
        }
        final Header contentLengthHeader = message.getFirstHeader(HTTP.CONTENT_LEN);
        if (contentLengthHeader != null) {
            final String s = contentLengthHeader.getValue();
            try {
                final long len = Long.parseLong(s);
                if (len < 0) {
                    throw new ProtocolException("Negative content length: " + s);
                }
                return len;
            } catch (final NumberFormatException e) {
                throw new ProtocolException("Invalid content length: " + s);
            }
        }
        return this.implicitLen;
    }

}
