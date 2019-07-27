

























package ch.boye.httpclientandroidlib.impl.auth;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AuthenticationException;







@Immutable
public class NTLMEngineException extends AuthenticationException {

    private static final long serialVersionUID = 6027981323731768824L;

    public NTLMEngineException() {
        super();
    }

    




    public NTLMEngineException(final String message) {
        super(message);
    }

    






    public NTLMEngineException(final String message, final Throwable cause) {
        super(message, cause);
    }

}
