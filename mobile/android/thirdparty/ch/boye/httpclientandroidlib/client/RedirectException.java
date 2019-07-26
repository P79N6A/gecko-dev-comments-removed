

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.ProtocolException;







@Immutable
public class RedirectException extends ProtocolException {

    private static final long serialVersionUID = 4418824536372559326L;

    


    public RedirectException() {
        super();
    }

    




    public RedirectException(String message) {
        super(message);
    }

    






    public RedirectException(String message, Throwable cause) {
        super(message, cause);
    }
}
