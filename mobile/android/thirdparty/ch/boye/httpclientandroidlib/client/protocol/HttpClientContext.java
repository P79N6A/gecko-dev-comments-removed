


























package ch.boye.httpclientandroidlib.client.protocol;

import java.net.URI;
import java.util.List;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.auth.AuthSchemeProvider;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.client.AuthCache;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.client.CredentialsProvider;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.config.Lookup;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.RouteInfo;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.CookieSpecProvider;
import ch.boye.httpclientandroidlib.protocol.BasicHttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.protocol.HttpCoreContext;








@NotThreadSafe
public class HttpClientContext extends HttpCoreContext {

    



    public static final String HTTP_ROUTE   = "http.route";

    



    public static final String REDIRECT_LOCATIONS = "http.protocol.redirect-locations";

    



    public static final String COOKIESPEC_REGISTRY   = "http.cookiespec-registry";

    



    public static final String COOKIE_SPEC           = "http.cookie-spec";

    



    public static final String COOKIE_ORIGIN         = "http.cookie-origin";

    



    public static final String COOKIE_STORE          = "http.cookie-store";

    



    public static final String CREDS_PROVIDER        = "http.auth.credentials-provider";

    



    public static final String AUTH_CACHE            = "http.auth.auth-cache";

    



    public static final String TARGET_AUTH_STATE     = "http.auth.target-scope";

    



    public static final String PROXY_AUTH_STATE      = "http.auth.proxy-scope";

    



    public static final String USER_TOKEN            = "http.user-token";

    



    public static final String AUTHSCHEME_REGISTRY   = "http.authscheme-registry";

    



    public static final String REQUEST_CONFIG = "http.request-config";

    public static HttpClientContext adapt(final HttpContext context) {
        if (context instanceof HttpClientContext) {
            return (HttpClientContext) context;
        } else {
            return new HttpClientContext(context);
        }
    }

    public static HttpClientContext create() {
        return new HttpClientContext(new BasicHttpContext());
    }

    public HttpClientContext(final HttpContext context) {
        super(context);
    }

    public HttpClientContext() {
        super();
    }

    public RouteInfo getHttpRoute() {
        return getAttribute(HTTP_ROUTE, HttpRoute.class);
    }

    @SuppressWarnings("unchecked")
    public List<URI> getRedirectLocations() {
        return getAttribute(REDIRECT_LOCATIONS, List.class);
    }

    public CookieStore getCookieStore() {
        return getAttribute(COOKIE_STORE, CookieStore.class);
    }

    public void setCookieStore(final CookieStore cookieStore) {
        setAttribute(COOKIE_STORE, cookieStore);
    }

    public CookieSpec getCookieSpec() {
        return getAttribute(COOKIE_SPEC, CookieSpec.class);
    }

    public CookieOrigin getCookieOrigin() {
        return getAttribute(COOKIE_ORIGIN, CookieOrigin.class);
    }

    @SuppressWarnings("unchecked")
    private <T> Lookup<T> getLookup(final String name, final Class<T> clazz) {
        return getAttribute(name, Lookup.class);
    }

    public Lookup<CookieSpecProvider> getCookieSpecRegistry() {
        return getLookup(COOKIESPEC_REGISTRY, CookieSpecProvider.class);
    }

    public void setCookieSpecRegistry(final Lookup<CookieSpecProvider> lookup) {
        setAttribute(COOKIESPEC_REGISTRY, lookup);
    }

    public Lookup<AuthSchemeProvider> getAuthSchemeRegistry() {
        return getLookup(AUTHSCHEME_REGISTRY, AuthSchemeProvider.class);
    }

    public void setAuthSchemeRegistry(final Lookup<AuthSchemeProvider> lookup) {
        setAttribute(AUTHSCHEME_REGISTRY, lookup);
    }

    public CredentialsProvider getCredentialsProvider() {
        return getAttribute(CREDS_PROVIDER, CredentialsProvider.class);
    }

    public void setCredentialsProvider(final CredentialsProvider credentialsProvider) {
        setAttribute(CREDS_PROVIDER, credentialsProvider);
    }

    public AuthCache getAuthCache() {
        return getAttribute(AUTH_CACHE, AuthCache.class);
    }

    public void setAuthCache(final AuthCache authCache) {
        setAttribute(AUTH_CACHE, authCache);
    }

    public AuthState getTargetAuthState() {
        return getAttribute(TARGET_AUTH_STATE, AuthState.class);
    }

    public AuthState getProxyAuthState() {
        return getAttribute(PROXY_AUTH_STATE, AuthState.class);
    }

    public <T> T getUserToken(final Class<T> clazz) {
        return getAttribute(USER_TOKEN, clazz);
    }

    public Object getUserToken() {
        return getAttribute(USER_TOKEN);
    }

    public void setUserToken(final Object obj) {
        setAttribute(USER_TOKEN, obj);
    }

    public RequestConfig getRequestConfig() {
        final RequestConfig config = getAttribute(REQUEST_CONFIG, RequestConfig.class);
        return config != null ? config : RequestConfig.DEFAULT;
    }

    public void setRequestConfig(final RequestConfig config) {
        setAttribute(REQUEST_CONFIG, config);
    }

}
