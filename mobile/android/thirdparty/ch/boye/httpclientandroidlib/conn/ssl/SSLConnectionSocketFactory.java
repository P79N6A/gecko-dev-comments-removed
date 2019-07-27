


























package ch.boye.httpclientandroidlib.conn.ssl;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.socket.LayeredConnectionSocketFactory;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.TextUtils;

import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;










































































@ThreadSafe
public class SSLConnectionSocketFactory implements LayeredConnectionSocketFactory {

    public static final String TLS   = "TLS";
    public static final String SSL   = "SSL";
    public static final String SSLV2 = "SSLv2";

    public static final X509HostnameVerifier ALLOW_ALL_HOSTNAME_VERIFIER
        = new AllowAllHostnameVerifier();

    public static final X509HostnameVerifier BROWSER_COMPATIBLE_HOSTNAME_VERIFIER
        = new BrowserCompatHostnameVerifier();

    public static final X509HostnameVerifier STRICT_HOSTNAME_VERIFIER
        = new StrictHostnameVerifier();

    






    public static SSLConnectionSocketFactory getSocketFactory() throws SSLInitializationException {
        return new SSLConnectionSocketFactory(
            SSLContexts.createDefault(),
            BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    private static String[] split(final String s) {
        if (TextUtils.isBlank(s)) {
            return null;
        }
        return s.split(" *, *");
    }

    








    public static SSLConnectionSocketFactory getSystemSocketFactory() throws SSLInitializationException {
        return new SSLConnectionSocketFactory(
            (javax.net.ssl.SSLSocketFactory) javax.net.ssl.SSLSocketFactory.getDefault(),
            split(System.getProperty("https.protocols")),
            split(System.getProperty("https.cipherSuites")),
            BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    private final javax.net.ssl.SSLSocketFactory socketfactory;
    private final X509HostnameVerifier hostnameVerifier;
    private final String[] supportedProtocols;
    private final String[] supportedCipherSuites;

    public SSLConnectionSocketFactory(final SSLContext sslContext) {
        this(sslContext, BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    public SSLConnectionSocketFactory(
            final SSLContext sslContext, final X509HostnameVerifier hostnameVerifier) {
        this(Args.notNull(sslContext, "SSL context").getSocketFactory(),
                null, null, hostnameVerifier);
    }

    public SSLConnectionSocketFactory(
            final SSLContext sslContext,
            final String[] supportedProtocols,
            final String[] supportedCipherSuites,
            final X509HostnameVerifier hostnameVerifier) {
        this(Args.notNull(sslContext, "SSL context").getSocketFactory(),
                supportedProtocols, supportedCipherSuites, hostnameVerifier);
    }

    public SSLConnectionSocketFactory(
            final javax.net.ssl.SSLSocketFactory socketfactory,
            final X509HostnameVerifier hostnameVerifier) {
        this(socketfactory, null, null, hostnameVerifier);
    }

    public SSLConnectionSocketFactory(
            final javax.net.ssl.SSLSocketFactory socketfactory,
            final String[] supportedProtocols,
            final String[] supportedCipherSuites,
            final X509HostnameVerifier hostnameVerifier) {
        this.socketfactory = Args.notNull(socketfactory, "SSL socket factory");
        this.supportedProtocols = supportedProtocols;
        this.supportedCipherSuites = supportedCipherSuites;
        this.hostnameVerifier = hostnameVerifier != null ? hostnameVerifier : BROWSER_COMPATIBLE_HOSTNAME_VERIFIER;
    }

    






    protected void prepareSocket(final SSLSocket socket) throws IOException {
    }

    public Socket createSocket(final HttpContext context) throws IOException {
        return SocketFactory.getDefault().createSocket();
    }

    public Socket connectSocket(
            final int connectTimeout,
            final Socket socket,
            final HttpHost host,
            final InetSocketAddress remoteAddress,
            final InetSocketAddress localAddress,
            final HttpContext context) throws IOException {
        Args.notNull(host, "HTTP host");
        Args.notNull(remoteAddress, "Remote address");
        final Socket sock = socket != null ? socket : createSocket(context);
        if (localAddress != null) {
            sock.bind(localAddress);
        }
        try {
            sock.connect(remoteAddress, connectTimeout);
        } catch (final IOException ex) {
            try {
                sock.close();
            } catch (final IOException ignore) {
            }
            throw ex;
        }
        
        if (sock instanceof SSLSocket) {
            final SSLSocket sslsock = (SSLSocket) sock;
            sslsock.startHandshake();
            verifyHostname(sslsock, host.getHostName());
            return sock;
        } else {
            return createLayeredSocket(sock, host.getHostName(), remoteAddress.getPort(), context);
        }
    }

    public Socket createLayeredSocket(
            final Socket socket,
            final String target,
            final int port,
            final HttpContext context) throws IOException {
        final SSLSocket sslsock = (SSLSocket) this.socketfactory.createSocket(
                socket,
                target,
                port,
                true);
        if (supportedProtocols != null) {
            sslsock.setEnabledProtocols(supportedProtocols);
        }
        if (supportedCipherSuites != null) {
            sslsock.setEnabledCipherSuites(supportedCipherSuites);
        }
        prepareSocket(sslsock);
        sslsock.startHandshake();
        verifyHostname(sslsock, target);
        return sslsock;
    }

    X509HostnameVerifier getHostnameVerifier() {
        return this.hostnameVerifier;
    }

    private void verifyHostname(final SSLSocket sslsock, final String hostname) throws IOException {
        try {
            this.hostnameVerifier.verify(hostname, sslsock);
            
        } catch (final IOException iox) {
            
            try { sslsock.close(); } catch (final Exception x) {  }
            throw iox;
        }
    }

}
