

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class RedirectException extends ProtocolException {

    private static final long serialVersionUID = 4418824536372559326L;

    


    public RedirectException() {
        super();
    }

    




    public RedirectException(final String message) {
        super(message);
    }

    






    public RedirectException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
