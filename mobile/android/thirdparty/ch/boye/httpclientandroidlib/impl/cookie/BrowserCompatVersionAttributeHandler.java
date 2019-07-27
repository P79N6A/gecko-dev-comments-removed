


























package ch.boye.httpclientandroidlib.impl.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class BrowserCompatVersionAttributeHandler extends
        AbstractCookieAttributeHandler {

    public BrowserCompatVersionAttributeHandler() {
        super();
    }

    


    public void parse(final SetCookie cookie, final String value)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (value == null) {
            throw new MalformedCookieException("Missing value for version attribute");
        }
        int version = 0;
        try {
            version = Integer.parseInt(value);
        } catch (final NumberFormatException e) {
            
        }
        cookie.setVersion(version);
    }

}
