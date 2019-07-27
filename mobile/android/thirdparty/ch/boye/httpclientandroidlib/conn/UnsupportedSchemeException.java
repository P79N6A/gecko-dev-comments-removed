


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class UnsupportedSchemeException extends IOException {

    private static final long serialVersionUID = 3597127619218687636L;

    


    public UnsupportedSchemeException(final String message) {
        super(message);
    }

}
