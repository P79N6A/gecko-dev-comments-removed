


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.util.Args;







@NotThreadSafe 
public abstract class CookieSpecBase extends AbstractCookieSpec {

    protected static String getDefaultPath(final CookieOrigin origin) {
        String defaultPath = origin.getPath();
        int lastSlashIndex = defaultPath.lastIndexOf('/');
        if (lastSlashIndex >= 0) {
            if (lastSlashIndex == 0) {
                
                lastSlashIndex = 1;
            }
            defaultPath = defaultPath.substring(0, lastSlashIndex);
        }
        return defaultPath;
    }

    protected static String getDefaultDomain(final CookieOrigin origin) {
        return origin.getHost();
    }

    protected List<Cookie> parse(final HeaderElement[] elems, final CookieOrigin origin)
                throws MalformedCookieException {
        final List<Cookie> cookies = new ArrayList<Cookie>(elems.length);
        for (final HeaderElement headerelement : elems) {
            final String name = headerelement.getName();
            final String value = headerelement.getValue();
            if (name == null || name.length() == 0) {
                throw new MalformedCookieException("Cookie name may not be empty");
            }

            final BasicClientCookie cookie = new BasicClientCookie(name, value);
            cookie.setPath(getDefaultPath(origin));
            cookie.setDomain(getDefaultDomain(origin));

            
            final NameValuePair[] attribs = headerelement.getParameters();
            for (int j = attribs.length - 1; j >= 0; j--) {
                final NameValuePair attrib = attribs[j];
                final String s = attrib.getName().toLowerCase(Locale.ENGLISH);

                cookie.setAttribute(s, attrib.getValue());

                final CookieAttributeHandler handler = findAttribHandler(s);
                if (handler != null) {
                    handler.parse(cookie, attrib.getValue());
                }
            }
            cookies.add(cookie);
        }
        return cookies;
    }

    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        for (final CookieAttributeHandler handler: getAttribHandlers()) {
            handler.validate(cookie, origin);
        }
    }

    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        for (final CookieAttributeHandler handler: getAttribHandlers()) {
            if (!handler.match(cookie, origin)) {
                return false;
            }
        }
        return true;
    }

}
