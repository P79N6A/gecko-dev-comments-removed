

























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.UserTokenHandler;
import ch.boye.httpclientandroidlib.protocol.HttpContext;






@Immutable
public class NoopUserTokenHandler implements UserTokenHandler {

    public static final NoopUserTokenHandler INSTANCE = new NoopUserTokenHandler();

    public Object getUserToken(final HttpContext context) {
        return null;
    }

}
