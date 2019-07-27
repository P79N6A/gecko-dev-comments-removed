


























package ch.boye.httpclientandroidlib.impl.client;

import java.util.Collection;

import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;







@Immutable
public class TargetAuthenticationStrategy extends AuthenticationStrategyImpl {

    public static final TargetAuthenticationStrategy INSTANCE = new TargetAuthenticationStrategy();

    public TargetAuthenticationStrategy() {
        super(HttpStatus.SC_UNAUTHORIZED, AUTH.WWW_AUTH);
    }

    @Override
    Collection<String> getPreferredAuthSchemes(final RequestConfig config) {
        return config.getTargetPreferredAuthSchemes();
    }

}
