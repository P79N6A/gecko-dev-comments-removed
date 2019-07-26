

























package ch.boye.httpclientandroidlib.impl.conn;

import java.net.InetAddress;
import java.net.InetSocketAddress;

import ch.boye.httpclientandroidlib.HttpHost;





class HttpInetSocketAddress extends InetSocketAddress {

    private static final long serialVersionUID = -6650701828361907957L;

    private final HttpHost host;

    public HttpInetSocketAddress(final HttpHost host, final InetAddress addr, int port) {
        super(addr, port);
        if (host == null) {
            throw new IllegalArgumentException("HTTP host may not be null");
        }
        this.host = host;
    }

    public HttpHost getHost() {
        return this.host;
    }

    @Override
    public String toString() {
        return this.host.getHostName() + ":" + getPort();
    }

}
