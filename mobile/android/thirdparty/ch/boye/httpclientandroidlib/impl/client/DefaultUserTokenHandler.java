

























package ch.boye.httpclientandroidlib.impl.client;

import java.security.Principal;

import javax.net.ssl.SSLSession;

import ch.boye.httpclientandroidlib.HttpConnection;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.auth.Credentials;
import ch.boye.httpclientandroidlib.client.UserTokenHandler;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.ManagedHttpClientConnection;
import ch.boye.httpclientandroidlib.protocol.HttpContext;















@Immutable
public class DefaultUserTokenHandler implements UserTokenHandler {

    public static final DefaultUserTokenHandler INSTANCE = new DefaultUserTokenHandler();

    public Object getUserToken(final HttpContext context) {

        final HttpClientContext clientContext = HttpClientContext.adapt(context);

        Principal userPrincipal = null;

        final AuthState targetAuthState = clientContext.getTargetAuthState();
        if (targetAuthState != null) {
            userPrincipal = getAuthPrincipal(targetAuthState);
            if (userPrincipal == null) {
                final AuthState proxyAuthState = clientContext.getProxyAuthState();
                userPrincipal = getAuthPrincipal(proxyAuthState);
            }
        }

        if (userPrincipal == null) {
            final HttpConnection conn = clientContext.getConnection();
            if (conn.isOpen() && conn instanceof ManagedHttpClientConnection) {
                final SSLSession sslsession = ((ManagedHttpClientConnection) conn).getSSLSession();
                if (sslsession != null) {
                    userPrincipal = sslsession.getLocalPrincipal();
                }
            }
        }

        return userPrincipal;
    }

    private static Principal getAuthPrincipal(final AuthState authState) {
        final AuthScheme scheme = authState.getAuthScheme();
        if (scheme != null && scheme.isComplete() && scheme.isConnectionBased()) {
            final Credentials creds = authState.getCredentials();
            if (creds != null) {
                return creds.getUserPrincipal();
            }
        }
        return null;
    }

}
