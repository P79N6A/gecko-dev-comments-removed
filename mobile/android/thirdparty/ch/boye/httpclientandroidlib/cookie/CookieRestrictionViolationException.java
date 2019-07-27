


























package ch.boye.httpclientandroidlib.cookie;

import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class CookieRestrictionViolationException extends MalformedCookieException {

    private static final long serialVersionUID = 7371235577078589013L;

    



    public CookieRestrictionViolationException() {
        super();
    }

    





    public CookieRestrictionViolationException(final String message) {
        super(message);
    }

}
