


























package ch.boye.httpclientandroidlib.impl.entity;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;










@Immutable
public class LaxContentLengthStrategy implements ContentLengthStrategy {

    public static final LaxContentLengthStrategy INSTANCE = new LaxContentLengthStrategy();

    private final int implicitLen;

    







    public LaxContentLengthStrategy(final int implicitLen) {
        super();
        this.implicitLen = implicitLen;
    }

    



    public LaxContentLengthStrategy() {
        this(IDENTITY);
    }

    public long determineLength(final HttpMessage message) throws HttpException {
        Args.notNull(message, "HTTP message");

        final Header transferEncodingHeader = message.getFirstHeader(HTTP.TRANSFER_ENCODING);
        
        
        if (transferEncodingHeader != null) {
            final HeaderElement[] encodings;
            try {
                encodings = transferEncodingHeader.getElements();
            } catch (final ParseException px) {
                throw new ProtocolException
                    ("Invalid Transfer-Encoding header value: " +
                     transferEncodingHeader, px);
            }
            
            final int len = encodings.length;
            if (HTTP.IDENTITY_CODING.equalsIgnoreCase(transferEncodingHeader.getValue())) {
                return IDENTITY;
            } else if ((len > 0) && (HTTP.CHUNK_CODING.equalsIgnoreCase(
                    encodings[len - 1].getName()))) {
                return CHUNKED;
            } else {
                return IDENTITY;
            }
        }
        final Header contentLengthHeader = message.getFirstHeader(HTTP.CONTENT_LEN);
        if (contentLengthHeader != null) {
            long contentlen = -1;
            final Header[] headers = message.getHeaders(HTTP.CONTENT_LEN);
            for (int i = headers.length - 1; i >= 0; i--) {
                final Header header = headers[i];
                try {
                    contentlen = Long.parseLong(header.getValue());
                    break;
                } catch (final NumberFormatException ignore) {
                }
                
            }
            if (contentlen >= 0) {
                return contentlen;
            } else {
                return IDENTITY;
            }
        }
        return this.implicitLen;
    }

}
