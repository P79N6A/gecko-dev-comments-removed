


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.client.AuthenticationStrategy;
import ch.boye.httpclientandroidlib.protocol.HttpContext;





@Deprecated
public class HttpAuthenticator extends ch.boye.httpclientandroidlib.impl.auth.HttpAuthenticator {

    public HttpAuthenticator(final HttpClientAndroidLog log) {
        super(log);
    }

    public HttpAuthenticator() {
        super();
    }

    public boolean authenticate (
            final HttpHost host,
            final HttpResponse response,
            final AuthenticationStrategy authStrategy,
            final AuthState authState,
            final HttpContext context) {
        return handleAuthChallenge(host, response, authStrategy, authState, context);
    }

}
