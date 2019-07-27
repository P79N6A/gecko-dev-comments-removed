

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class CircularRedirectException extends RedirectException {

    private static final long serialVersionUID = 6830063487001091803L;

    


    public CircularRedirectException() {
        super();
    }

    




    public CircularRedirectException(final String message) {
        super(message);
    }

    






    public CircularRedirectException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
