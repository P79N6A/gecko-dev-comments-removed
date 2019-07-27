

























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;





@Immutable
public abstract class AbstractCookieAttributeHandler implements CookieAttributeHandler {

    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        
    }

    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        
        return true;
    }

}
