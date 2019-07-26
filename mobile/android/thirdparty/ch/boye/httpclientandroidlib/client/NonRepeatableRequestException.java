

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.ProtocolException;








@Immutable
public class NonRepeatableRequestException extends ProtocolException {

    private static final long serialVersionUID = 82685265288806048L;

    


    public NonRepeatableRequestException() {
        super();
    }

    




    public NonRepeatableRequestException(String message) {
        super(message);
    }

    





    public NonRepeatableRequestException(String message, Throwable cause) {
        super(message, cause);
    }



}
