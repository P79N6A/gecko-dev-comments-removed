

























package ch.boye.httpclientandroidlib.auth;

import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class MalformedChallengeException extends ProtocolException {

    private static final long serialVersionUID = 814586927989932284L;

    


    public MalformedChallengeException() {
        super();
    }

    




    public MalformedChallengeException(final String message) {
        super(message);
    }

    






    public MalformedChallengeException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
