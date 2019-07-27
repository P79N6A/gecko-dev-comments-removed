

























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class AuthenticationException extends ProtocolException {

    private static final long serialVersionUID = -6794031905674764776L;

    


    public AuthenticationException() {
        super();
    }

    




    public AuthenticationException(final String message) {
        super(message);
    }

    






    public AuthenticationException(final String message, final Throwable cause) {
        super(message, cause);
    }

}
