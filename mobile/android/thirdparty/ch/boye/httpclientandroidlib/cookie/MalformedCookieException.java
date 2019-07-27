


























package ch.boye.httpclientandroidlib.cookie;

import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class MalformedCookieException extends ProtocolException {

    private static final long serialVersionUID = -6695462944287282185L;

    


    public MalformedCookieException() {
        super();
    }

    




    public MalformedCookieException(final String message) {
        super(message);
    }

    






    public MalformedCookieException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
