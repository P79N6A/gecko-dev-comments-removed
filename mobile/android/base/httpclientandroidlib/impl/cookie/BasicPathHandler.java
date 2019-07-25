

























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieRestrictionViolationException;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;





@Immutable
public class BasicPathHandler implements CookieAttributeHandler {

    public BasicPathHandler() {
        super();
    }

    public void parse(final SetCookie cookie, String value)
            throws MalformedCookieException {
        if (cookie == null) {
            throw new IllegalArgumentException("Cookie may not be null");
        }
        if (value == null || value.trim().length() == 0) {
            value = "/";
        }
        cookie.setPath(value);
    }

    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        if (!match(cookie, origin)) {
            throw new CookieRestrictionViolationException(
                "Illegal path attribute \"" + cookie.getPath()
                + "\". Path of origin: \"" + origin.getPath() + "\"");
        }
    }

    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        if (cookie == null) {
            throw new IllegalArgumentException("Cookie may not be null");
        }
        if (origin == null) {
            throw new IllegalArgumentException("Cookie origin may not be null");
        }
        String targetpath = origin.getPath();
        String topmostPath = cookie.getPath();
        if (topmostPath == null) {
            topmostPath = "/";
        }
        if (topmostPath.length() > 1 && topmostPath.endsWith("/")) {
            topmostPath = topmostPath.substring(0, topmostPath.length() - 1);
        }
        boolean match = targetpath.startsWith (topmostPath);
        
        
        if (match && targetpath.length() != topmostPath.length()) {
            if (!topmostPath.endsWith("/")) {
                match = (targetpath.charAt(topmostPath.length()) == '/');
            }
        }
        return match;
    }

}
