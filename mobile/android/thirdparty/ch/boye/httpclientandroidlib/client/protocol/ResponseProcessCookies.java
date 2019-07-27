


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;
import java.util.List;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SM;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class ResponseProcessCookies implements HttpResponseInterceptor {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    public ResponseProcessCookies() {
        super();
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP request");
        Args.notNull(context, "HTTP context");

        final HttpClientContext clientContext = HttpClientContext.adapt(context);

        
        final CookieSpec cookieSpec = clientContext.getCookieSpec();
        if (cookieSpec == null) {
            this.log.debug("Cookie spec not specified in HTTP context");
            return;
        }
        
        final CookieStore cookieStore = clientContext.getCookieStore();
        if (cookieStore == null) {
            this.log.debug("Cookie store not specified in HTTP context");
            return;
        }
        
        final CookieOrigin cookieOrigin = clientContext.getCookieOrigin();
        if (cookieOrigin == null) {
            this.log.debug("Cookie origin not specified in HTTP context");
            return;
        }
        HeaderIterator it = response.headerIterator(SM.SET_COOKIE);
        processCookies(it, cookieSpec, cookieOrigin, cookieStore);

        
        if (cookieSpec.getVersion() > 0) {
            
            
            it = response.headerIterator(SM.SET_COOKIE2);
            processCookies(it, cookieSpec, cookieOrigin, cookieStore);
        }
    }

    private void processCookies(
            final HeaderIterator iterator,
            final CookieSpec cookieSpec,
            final CookieOrigin cookieOrigin,
            final CookieStore cookieStore) {
        while (iterator.hasNext()) {
            final Header header = iterator.nextHeader();
            try {
                final List<Cookie> cookies = cookieSpec.parse(header, cookieOrigin);
                for (final Cookie cookie : cookies) {
                    try {
                        cookieSpec.validate(cookie, cookieOrigin);
                        cookieStore.addCookie(cookie);

                        if (this.log.isDebugEnabled()) {
                            this.log.debug("Cookie accepted [" + formatCooke(cookie) + "]");
                        }
                    } catch (final MalformedCookieException ex) {
                        if (this.log.isWarnEnabled()) {
                            this.log.warn("Cookie rejected [" + formatCooke(cookie) + "] "
                                    + ex.getMessage());
                        }
                    }
                }
            } catch (final MalformedCookieException ex) {
                if (this.log.isWarnEnabled()) {
                    this.log.warn("Invalid cookie header: \""
                            + header + "\". " + ex.getMessage());
                }
            }
        }
    }

    private static String formatCooke(final Cookie cookie) {
        final StringBuilder buf = new StringBuilder();
        buf.append(cookie.getName());
        buf.append("=\"");
        String v = cookie.getValue();
        if (v.length() > 100) {
            v = v.substring(0, 100) + "...";
        }
        buf.append(v);
        buf.append("\"");
        buf.append(", version:");
        buf.append(Integer.toString(cookie.getVersion()));
        buf.append(", domain:");
        buf.append(cookie.getDomain());
        buf.append(", path:");
        buf.append(cookie.getPath());
        buf.append(", expiry:");
        buf.append(cookie.getExpiryDate());
        return buf.toString();
    }

}
