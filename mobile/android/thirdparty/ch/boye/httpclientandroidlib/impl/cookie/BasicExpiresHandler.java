

























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Date;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.util.Args;





@Immutable
public class BasicExpiresHandler extends AbstractCookieAttributeHandler {

    
    private final String[] datepatterns;

    public BasicExpiresHandler(final String[] datepatterns) {
        Args.notNull(datepatterns, "Array of date patterns");
        this.datepatterns = datepatterns;
    }

    public void parse(final SetCookie cookie, final String value)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (value == null) {
            throw new MalformedCookieException("Missing value for expires attribute");
        }
        final Date expiry = DateUtils.parseDate(value, this.datepatterns);
        if (expiry == null) {
            throw new MalformedCookieException("Unable to parse expires attribute: "
                    + value);
        }
        cookie.setExpiryDate(expiry);
    }

}
