


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Collections;
import java.util.List;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;






@NotThreadSafe 
public class IgnoreSpec extends CookieSpecBase {

    public int getVersion() {
        return 0;
    }

    public List<Cookie> parse(final Header header, final CookieOrigin origin)
            throws MalformedCookieException {
        return Collections.emptyList();
    }

    public List<Header> formatCookies(final List<Cookie> cookies) {
        return Collections.emptyList();
    }

    public Header getVersionHeader() {
        return null;
    }
}
