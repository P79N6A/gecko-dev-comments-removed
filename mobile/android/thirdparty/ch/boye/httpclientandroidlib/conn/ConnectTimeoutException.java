


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.InetAddress;
import java.util.Arrays;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;








@Immutable
public class ConnectTimeoutException extends InterruptedIOException {

    private static final long serialVersionUID = -4816682903149535989L;

    private final HttpHost host;

    


    public ConnectTimeoutException() {
        super();
        this.host = null;
    }

    


    public ConnectTimeoutException(final String message) {
        super(message);
        this.host = null;
    }

    




    public ConnectTimeoutException(
            final IOException cause,
            final HttpHost host,
            final InetAddress... remoteAddresses) {
        super("Connect to " +
                (host != null ? host.toHostString() : "remote host") +
                (remoteAddresses != null && remoteAddresses.length > 0 ?
                        " " + Arrays.asList(remoteAddresses) : "") +
                ((cause != null && cause.getMessage() != null) ?
                        " failed: " + cause.getMessage() : " timed out"));
        this.host = host;
        initCause(cause);
    }

    


    public HttpHost getHost() {
        return host;
    }

}
