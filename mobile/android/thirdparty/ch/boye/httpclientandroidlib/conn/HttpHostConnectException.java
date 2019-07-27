

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.ConnectException;
import java.net.InetAddress;
import java.util.Arrays;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class HttpHostConnectException extends ConnectException {

    private static final long serialVersionUID = -3194482710275220224L;

    private final HttpHost host;

    



    @Deprecated
    public HttpHostConnectException(final HttpHost host, final ConnectException cause) {
        this(cause, host, null);
    }

    




    public HttpHostConnectException(
            final IOException cause,
            final HttpHost host,
            final InetAddress... remoteAddresses) {
        super("Connect to " +
                (host != null ? host.toHostString() : "remote host") +
                (remoteAddresses != null && remoteAddresses .length > 0 ?
                        " " + Arrays.asList(remoteAddresses) : "") +
                ((cause != null && cause.getMessage() != null) ?
                        " failed: " + cause.getMessage() : " refused"));
        this.host = host;
        initCause(cause);
    }

    public HttpHost getHost() {
        return this.host;
    }

}
