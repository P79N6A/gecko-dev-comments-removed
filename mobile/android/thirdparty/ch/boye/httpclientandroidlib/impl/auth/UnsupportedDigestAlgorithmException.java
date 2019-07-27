

























package ch.boye.httpclientandroidlib.impl.auth;

import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class UnsupportedDigestAlgorithmException extends RuntimeException {

    private static final long serialVersionUID = 319558534317118022L;

    


    public UnsupportedDigestAlgorithmException() {
        super();
    }

    




    public UnsupportedDigestAlgorithmException(final String message) {
        super(message);
    }

    






    public UnsupportedDigestAlgorithmException(final String message, final Throwable cause) {
        super(message, cause);
    }
}
