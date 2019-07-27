


























package ch.boye.httpclientandroidlib.client.protocol;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthSchemeRegistry;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.cookie.CookieSpecRegistry;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;








@NotThreadSafe
@Deprecated
public class ClientContextConfigurer implements ClientContext {

    private final HttpContext context;

    public ClientContextConfigurer (final HttpContext context) {
        Args.notNull(context, "HTTP context");
        this.context = context;
    }

    public void setCookieSpecRegistry(final CookieSpecRegistry registry) {
        this.context.setAttribute(COOKIESPEC_REGISTRY, registry);
    }

    public void setAuthSchemeRegistry(final AuthSchemeRegistry registry) {
        this.context.setAttribute(AUTHSCHEME_REGISTRY, registry);
    }

    public void setCookieStore(final CookieStore store) {
        this.context.setAttribute(COOKIE_STORE, store);
    }

    public void setCredentialsProvider(final CredentialsProvider provider) {
        this.context.setAttribute(CREDS_PROVIDER, provider);
    }

}
