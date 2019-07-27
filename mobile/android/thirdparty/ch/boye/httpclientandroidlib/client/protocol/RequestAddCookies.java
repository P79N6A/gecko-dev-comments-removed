


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.client.config.CookieSpecs;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.config.Lookup;
import ch.boye.httpclientandroidlib.conn.routing.RouteInfo;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.CookieSpecProvider;
import ch.boye.httpclientandroidlib.cookie.SetCookie2;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.TextUtils;








@Immutable
public class RequestAddCookies implements HttpRequestInterceptor {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    public RequestAddCookies() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");

        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT")) {
            return;
        }

        final HttpClientContext clientContext = HttpClientContext.adapt(context);

        
        final CookieStore cookieStore = clientContext.getCookieStore();
        if (cookieStore == null) {
            this.log.debug("Cookie store not specified in HTTP context");
            return;
        }

        
        final Lookup<CookieSpecProvider> registry = clientContext.getCookieSpecRegistry();
        if (registry == null) {
            this.log.debug("CookieSpec registry not specified in HTTP context");
            return;
        }

        
        final HttpHost targetHost = clientContext.getTargetHost();
        if (targetHost == null) {
            this.log.debug("Target host not set in the context");
            return;
        }

        
        final RouteInfo route = clientContext.getHttpRoute();
        if (route == null) {
            this.log.debug("Connection route not set in the context");
            return;
        }

        final RequestConfig config = clientContext.getRequestConfig();
        String policy = config.getCookieSpec();
        if (policy == null) {
            policy = CookieSpecs.BEST_MATCH;
        }
        if (this.log.isDebugEnabled()) {
            this.log.debug("CookieSpec selected: " + policy);
        }

        URI requestURI = null;
        if (request instanceof HttpUriRequest) {
            requestURI = ((HttpUriRequest) request).getURI();
        } else {
            try {
                requestURI = new URI(request.getRequestLine().getUri());
            } catch (final URISyntaxException ignore) {
            }
        }
        final String path = requestURI != null ? requestURI.getPath() : null;
        final String hostName = targetHost.getHostName();
        int port = targetHost.getPort();
        if (port < 0) {
            port = route.getTargetHost().getPort();
        }

        final CookieOrigin cookieOrigin = new CookieOrigin(
                hostName,
                port >= 0 ? port : 0,
                !TextUtils.isEmpty(path) ? path : "/",
                route.isSecure());

        
        final CookieSpecProvider provider = registry.lookup(policy);
        if (provider == null) {
            throw new HttpException("Unsupported cookie policy: " + policy);
        }
        final CookieSpec cookieSpec = provider.create(clientContext);
        
        final List<Cookie> cookies = new ArrayList<Cookie>(cookieStore.getCookies());
        
        final List<Cookie> matchedCookies = new ArrayList<Cookie>();
        final Date now = new Date();
        for (final Cookie cookie : cookies) {
            if (!cookie.isExpired(now)) {
                if (cookieSpec.match(cookie, cookieOrigin)) {
                    if (this.log.isDebugEnabled()) {
                        this.log.debug("Cookie " + cookie + " match " + cookieOrigin);
                    }
                    matchedCookies.add(cookie);
                }
            } else {
                if (this.log.isDebugEnabled()) {
                    this.log.debug("Cookie " + cookie + " expired");
                }
            }
        }
        
        if (!matchedCookies.isEmpty()) {
            final List<Header> headers = cookieSpec.formatCookies(matchedCookies);
            for (final Header header : headers) {
                request.addHeader(header);
            }
        }

        final int ver = cookieSpec.getVersion();
        if (ver > 0) {
            boolean needVersionHeader = false;
            for (final Cookie cookie : matchedCookies) {
                if (ver != cookie.getVersion() || !(cookie instanceof SetCookie2)) {
                    needVersionHeader = true;
                }
            }

            if (needVersionHeader) {
                final Header header = cookieSpec.getVersionHeader();
                if (header != null) {
                    
                    request.addHeader(header);
                }
            }
        }

        
        
        context.setAttribute(HttpClientContext.COOKIE_SPEC, cookieSpec);
        context.setAttribute(HttpClientContext.COOKIE_ORIGIN, cookieOrigin);
    }

}
