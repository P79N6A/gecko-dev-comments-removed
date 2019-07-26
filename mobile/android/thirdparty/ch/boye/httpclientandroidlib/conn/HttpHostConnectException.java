

























package ch.boye.httpclientandroidlib.conn;

import java.net.ConnectException;

import ch.boye.httpclientandroidlib.annotation.Immutable;

import ch.boye.httpclientandroidlib.HttpHost;







@Immutable
public class HttpHostConnectException extends ConnectException {

    private static final long serialVersionUID = -3194482710275220224L;

    private final HttpHost host;

    public HttpHostConnectException(final HttpHost host, final ConnectException cause) {
        super("Connection to " + host + " refused");
        this.host = host;
        initCause(cause);
    }

    public HttpHost getHost() {
        return this.host;
    }

}
