

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class NonRepeatableRequestException extends ProtocolException {

    private static final long serialVersionUID = 82685265288806048L;

    


    public NonRepeatableRequestException() {
        super();
    }

    




    public NonRepeatableRequestException(final String message) {
        super(message);
    }

    





    public NonRepeatableRequestException(final String message, final Throwable cause) {
        super(message, cause);
    }



}
