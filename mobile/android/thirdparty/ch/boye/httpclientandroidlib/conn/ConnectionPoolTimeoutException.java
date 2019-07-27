


























package ch.boye.httpclientandroidlib.conn;

import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class ConnectionPoolTimeoutException extends ConnectTimeoutException {

    private static final long serialVersionUID = -7898874842020245128L;

    


    public ConnectionPoolTimeoutException() {
        super();
    }

    




    public ConnectionPoolTimeoutException(final String message) {
        super(message);
    }

}
