


























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.ClientCookie;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieRestrictionViolationException;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.cookie.SetCookie2;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class RFC2965VersionAttributeHandler implements CookieAttributeHandler {

    public RFC2965VersionAttributeHandler() {
        super();
    }

    


    public void parse(final SetCookie cookie, final String value)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (value == null) {
            throw new MalformedCookieException(
                    "Missing value for version attribute");
        }
        int version = -1;
        try {
            version = Integer.parseInt(value);
        } catch (final NumberFormatException e) {
            version = -1;
        }
        if (version < 0) {
            throw new MalformedCookieException("Invalid cookie version.");
        }
        cookie.setVersion(version);
    }

    


    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (cookie instanceof SetCookie2) {
            if (cookie instanceof ClientCookie
                    && !((ClientCookie) cookie).containsAttribute(ClientCookie.VERSION_ATTR)) {
                throw new CookieRestrictionViolationException(
                        "Violates RFC 2965. Version attribute is required.");
            }
        }
    }

    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        return true;
    }

}
