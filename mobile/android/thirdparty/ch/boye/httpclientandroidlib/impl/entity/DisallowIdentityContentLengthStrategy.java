


























package ch.boye.httpclientandroidlib.impl.entity;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;







@Immutable
public class DisallowIdentityContentLengthStrategy implements ContentLengthStrategy {

    public static final DisallowIdentityContentLengthStrategy INSTANCE =
        new DisallowIdentityContentLengthStrategy(new LaxContentLengthStrategy(0));

    private final ContentLengthStrategy contentLengthStrategy;

    public DisallowIdentityContentLengthStrategy(final ContentLengthStrategy contentLengthStrategy) {
        super();
        this.contentLengthStrategy = contentLengthStrategy;
    }

    public long determineLength(final HttpMessage message) throws HttpException {
        final long result = this.contentLengthStrategy.determineLength(message);
        if (result == ContentLengthStrategy.IDENTITY) {
            throw new ProtocolException("Identity transfer encoding cannot be used");
        }
        return result;
    }

}
