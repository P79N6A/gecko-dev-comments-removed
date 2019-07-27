


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthScheme;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.client.AuthCache;
import ch.boye.httpclientandroidlib.client.params.AuthPolicy;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.impl.client.BasicAuthCache;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;











@Immutable
@Deprecated
public class ResponseAuthCache implements HttpResponseInterceptor {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    public ResponseAuthCache() {
        super();
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP request");
        Args.notNull(context, "HTTP context");
        AuthCache authCache = (AuthCache) context.getAttribute(ClientContext.AUTH_CACHE);

        HttpHost target = (HttpHost) context.getAttribute(ExecutionContext.HTTP_TARGET_HOST);
        final AuthState targetState = (AuthState) context.getAttribute(ClientContext.TARGET_AUTH_STATE);
        if (target != null && targetState != null) {
            if (this.log.isDebugEnabled()) {
                this.log.debug("Target auth state: " + targetState.getState());
            }
            if (isCachable(targetState)) {
                final SchemeRegistry schemeRegistry = (SchemeRegistry) context.getAttribute(
                        ClientContext.SCHEME_REGISTRY);
                if (target.getPort() < 0) {
                    final Scheme scheme = schemeRegistry.getScheme(target);
                    target = new HttpHost(target.getHostName(),
                            scheme.resolvePort(target.getPort()), target.getSchemeName());
                }
                if (authCache == null) {
                    authCache = new BasicAuthCache();
                    context.setAttribute(ClientContext.AUTH_CACHE, authCache);
                }
                switch (targetState.getState()) {
                case CHALLENGED:
                    cache(authCache, target, targetState.getAuthScheme());
                    break;
                case FAILURE:
                    uncache(authCache, target, targetState.getAuthScheme());
                }
            }
        }

        final HttpHost proxy = (HttpHost) context.getAttribute(ExecutionContext.HTTP_PROXY_HOST);
        final AuthState proxyState = (AuthState) context.getAttribute(ClientContext.PROXY_AUTH_STATE);
        if (proxy != null && proxyState != null) {
            if (this.log.isDebugEnabled()) {
                this.log.debug("Proxy auth state: " + proxyState.getState());
            }
            if (isCachable(proxyState)) {
                if (authCache == null) {
                    authCache = new BasicAuthCache();
                    context.setAttribute(ClientContext.AUTH_CACHE, authCache);
                }
                switch (proxyState.getState()) {
                case CHALLENGED:
                    cache(authCache, proxy, proxyState.getAuthScheme());
                    break;
                case FAILURE:
                    uncache(authCache, proxy, proxyState.getAuthScheme());
                }
            }
        }
    }

    private boolean isCachable(final AuthState authState) {
        final AuthScheme authScheme = authState.getAuthScheme();
        if (authScheme == null || !authScheme.isComplete()) {
            return false;
        }
        final String schemeName = authScheme.getSchemeName();
        return schemeName.equalsIgnoreCase(AuthPolicy.BASIC) ||
                schemeName.equalsIgnoreCase(AuthPolicy.DIGEST);
    }

    private void cache(final AuthCache authCache, final HttpHost host, final AuthScheme authScheme) {
        if (this.log.isDebugEnabled()) {
            this.log.debug("Caching '" + authScheme.getSchemeName() +
                    "' auth scheme for " + host);
        }
        authCache.put(host, authScheme);
    }

    private void uncache(final AuthCache authCache, final HttpHost host, final AuthScheme authScheme) {
        if (this.log.isDebugEnabled()) {
            this.log.debug("Removing from cache '" + authScheme.getSchemeName() +
                    "' auth scheme for " + host);
        }
        authCache.remove(host);
    }
}
