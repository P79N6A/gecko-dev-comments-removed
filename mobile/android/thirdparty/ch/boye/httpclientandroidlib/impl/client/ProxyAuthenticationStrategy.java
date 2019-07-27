


























package ch.boye.httpclientandroidlib.impl.client;

import java.util.Collection;

import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;







@Immutable
public class ProxyAuthenticationStrategy extends AuthenticationStrategyImpl {

    public static final ProxyAuthenticationStrategy INSTANCE = new ProxyAuthenticationStrategy();

    public ProxyAuthenticationStrategy() {
        super(HttpStatus.SC_PROXY_AUTHENTICATION_REQUIRED, AUTH.PROXY_AUTH);
    }

    @Override
    Collection<String> getPreferredAuthSchemes(final RequestConfig config) {
        return config.getProxyPreferredAuthSchemes();
    }

}
