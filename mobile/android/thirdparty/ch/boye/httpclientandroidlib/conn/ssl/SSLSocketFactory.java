


























package ch.boye.httpclientandroidlib.conn.ssl;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.ConnectTimeoutException;
import ch.boye.httpclientandroidlib.conn.HttpInetSocketAddress;
import ch.boye.httpclientandroidlib.conn.scheme.HostNameResolver;
import ch.boye.httpclientandroidlib.conn.scheme.LayeredSchemeSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.LayeredSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeLayeredSocketFactory;
import ch.boye.httpclientandroidlib.conn.socket.LayeredConnectionSocketFactory;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;
import ch.boye.httpclientandroidlib.util.TextUtils;












































































@ThreadSafe
@Deprecated
public class SSLSocketFactory implements LayeredConnectionSocketFactory, SchemeLayeredSocketFactory,
                                         LayeredSchemeSocketFactory, LayeredSocketFactory {

    public static final String TLS   = "TLS";
    public static final String SSL   = "SSL";
    public static final String SSLV2 = "SSLv2";

    public static final X509HostnameVerifier ALLOW_ALL_HOSTNAME_VERIFIER
        = new AllowAllHostnameVerifier();

    public static final X509HostnameVerifier BROWSER_COMPATIBLE_HOSTNAME_VERIFIER
        = new BrowserCompatHostnameVerifier();

    public static final X509HostnameVerifier STRICT_HOSTNAME_VERIFIER
        = new StrictHostnameVerifier();

    






    public static SSLSocketFactory getSocketFactory() throws SSLInitializationException {
        return new SSLSocketFactory(
            SSLContexts.createDefault(),
            BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    private static String[] split(final String s) {
        if (TextUtils.isBlank(s)) {
            return null;
        }
        return s.split(" *, *");
    }

    








    public static SSLSocketFactory getSystemSocketFactory() throws SSLInitializationException {
        return new SSLSocketFactory(
            (javax.net.ssl.SSLSocketFactory) javax.net.ssl.SSLSocketFactory.getDefault(),
            split(System.getProperty("https.protocols")),
            split(System.getProperty("https.cipherSuites")),
            BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    private final javax.net.ssl.SSLSocketFactory socketfactory;
    private final HostNameResolver nameResolver;
    
    private volatile X509HostnameVerifier hostnameVerifier;
    private final String[] supportedProtocols;
    private final String[] supportedCipherSuites;

    public SSLSocketFactory(
            final String algorithm,
            final KeyStore keystore,
            final String keyPassword,
            final KeyStore truststore,
            final SecureRandom random,
            final HostNameResolver nameResolver)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .useProtocol(algorithm)
                .setSecureRandom(random)
                .loadKeyMaterial(keystore, keyPassword != null ? keyPassword.toCharArray() : null)
                .loadTrustMaterial(truststore)
                .build(),
                nameResolver);
    }

    


    public SSLSocketFactory(
            final String algorithm,
            final KeyStore keystore,
            final String keyPassword,
            final KeyStore truststore,
            final SecureRandom random,
            final TrustStrategy trustStrategy,
            final X509HostnameVerifier hostnameVerifier)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .useProtocol(algorithm)
                .setSecureRandom(random)
                .loadKeyMaterial(keystore, keyPassword != null ? keyPassword.toCharArray() : null)
                .loadTrustMaterial(truststore, trustStrategy)
                .build(),
                hostnameVerifier);
    }

    


    public SSLSocketFactory(
            final String algorithm,
            final KeyStore keystore,
            final String keyPassword,
            final KeyStore truststore,
            final SecureRandom random,
            final X509HostnameVerifier hostnameVerifier)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .useProtocol(algorithm)
                .setSecureRandom(random)
                .loadKeyMaterial(keystore, keyPassword != null ? keyPassword.toCharArray() : null)
                .loadTrustMaterial(truststore)
                .build(),
                hostnameVerifier);
    }

    public SSLSocketFactory(
            final KeyStore keystore,
            final String keystorePassword,
            final KeyStore truststore)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .loadKeyMaterial(keystore, keystorePassword != null ? keystorePassword.toCharArray() : null)
                .loadTrustMaterial(truststore)
                .build(),
                BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    public SSLSocketFactory(
            final KeyStore keystore,
            final String keystorePassword)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException{
        this(SSLContexts.custom()
                .loadKeyMaterial(keystore, keystorePassword != null ? keystorePassword.toCharArray() : null)
                .build(),
                BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    public SSLSocketFactory(
            final KeyStore truststore)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .loadTrustMaterial(truststore)
                .build(),
                BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    


    public SSLSocketFactory(
            final TrustStrategy trustStrategy,
            final X509HostnameVerifier hostnameVerifier)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .loadTrustMaterial(null, trustStrategy)
                .build(),
                hostnameVerifier);
    }

    


    public SSLSocketFactory(
            final TrustStrategy trustStrategy)
                throws NoSuchAlgorithmException, KeyManagementException, KeyStoreException, UnrecoverableKeyException {
        this(SSLContexts.custom()
                .loadTrustMaterial(null, trustStrategy)
                .build(),
                BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    public SSLSocketFactory(final SSLContext sslContext) {
        this(sslContext, BROWSER_COMPATIBLE_HOSTNAME_VERIFIER);
    }

    public SSLSocketFactory(
            final SSLContext sslContext, final HostNameResolver nameResolver) {
        super();
        this.socketfactory = sslContext.getSocketFactory();
        this.hostnameVerifier = BROWSER_COMPATIBLE_HOSTNAME_VERIFIER;
        this.nameResolver = nameResolver;
        this.supportedProtocols = null;
        this.supportedCipherSuites = null;
    }

    


    public SSLSocketFactory(
            final SSLContext sslContext, final X509HostnameVerifier hostnameVerifier) {
        this(Args.notNull(sslContext, "SSL context").getSocketFactory(),
                null, null, hostnameVerifier);
    }

    


    public SSLSocketFactory(
            final SSLContext sslContext,
            final String[] supportedProtocols,
            final String[] supportedCipherSuites,
            final X509HostnameVerifier hostnameVerifier) {
        this(Args.notNull(sslContext, "SSL context").getSocketFactory(),
                supportedProtocols, supportedCipherSuites, hostnameVerifier);
    }

    


    public SSLSocketFactory(
            final javax.net.ssl.SSLSocketFactory socketfactory,
            final X509HostnameVerifier hostnameVerifier) {
        this(socketfactory, null, null, hostnameVerifier);
    }

    


    public SSLSocketFactory(
            final javax.net.ssl.SSLSocketFactory socketfactory,
            final String[] supportedProtocols,
            final String[] supportedCipherSuites,
            final X509HostnameVerifier hostnameVerifier) {
        this.socketfactory = Args.notNull(socketfactory, "SSL socket factory");
        this.supportedProtocols = supportedProtocols;
        this.supportedCipherSuites = supportedCipherSuites;
        this.hostnameVerifier = hostnameVerifier != null ? hostnameVerifier : BROWSER_COMPATIBLE_HOSTNAME_VERIFIER;
        this.nameResolver = null;
    }

    




    public Socket createSocket(final HttpParams params) throws IOException {
        return createSocket((HttpContext) null);
    }

    public Socket createSocket() throws IOException {
        return createSocket((HttpContext) null);
    }

    


    public Socket connectSocket(
            final Socket socket,
            final InetSocketAddress remoteAddress,
            final InetSocketAddress localAddress,
            final HttpParams params) throws IOException, UnknownHostException, ConnectTimeoutException {
        Args.notNull(remoteAddress, "Remote address");
        Args.notNull(params, "HTTP parameters");
        final HttpHost host;
        if (remoteAddress instanceof HttpInetSocketAddress) {
            host = ((HttpInetSocketAddress) remoteAddress).getHttpHost();
        } else {
            host = new HttpHost(remoteAddress.getHostName(), remoteAddress.getPort(), "https");
        }
        final int socketTimeout = HttpConnectionParams.getSoTimeout(params);
        final int connectTimeout = HttpConnectionParams.getConnectionTimeout(params);
        socket.setSoTimeout(socketTimeout);
        return connectSocket(connectTimeout, socket, host, remoteAddress, localAddress, null);
    }

    













    public boolean isSecure(final Socket sock) throws IllegalArgumentException {
        Args.notNull(sock, "Socket");
        Asserts.check(sock instanceof SSLSocket, "Socket not created by this factory");
        Asserts.check(!sock.isClosed(), "Socket is closed");
        return true;
    }

    


    public Socket createLayeredSocket(
        final Socket socket,
        final String host,
        final int port,
        final HttpParams params) throws IOException, UnknownHostException {
        return createLayeredSocket(socket, host, port, (HttpContext) null);
    }

    public Socket createLayeredSocket(
        final Socket socket,
        final String host,
        final int port,
        final boolean autoClose) throws IOException, UnknownHostException {
        return createLayeredSocket(socket, host, port, (HttpContext) null);
    }

    public void setHostnameVerifier(final X509HostnameVerifier hostnameVerifier) {
        Args.notNull(hostnameVerifier, "Hostname verifier");
        this.hostnameVerifier = hostnameVerifier;
    }

    public X509HostnameVerifier getHostnameVerifier() {
        return this.hostnameVerifier;
    }

    public Socket connectSocket(
            final Socket socket,
            final String host, final int port,
            final InetAddress local, final int localPort,
            final HttpParams params) throws IOException, UnknownHostException, ConnectTimeoutException {
        final InetAddress remote;
        if (this.nameResolver != null) {
            remote = this.nameResolver.resolve(host);
        } else {
            remote = InetAddress.getByName(host);
        }
        InetSocketAddress localAddress = null;
        if (local != null || localPort > 0) {
            localAddress = new InetSocketAddress(local, localPort > 0 ? localPort : 0);
        }
        final InetSocketAddress remoteAddress = new HttpInetSocketAddress(
                new HttpHost(host, port), remote, port);
        return connectSocket(socket, remoteAddress, localAddress, params);
    }

    public Socket createSocket(
            final Socket socket,
            final String host, final int port,
            final boolean autoClose) throws IOException, UnknownHostException {
        return createLayeredSocket(socket, host, port, autoClose);
    }

    








    protected void prepareSocket(final SSLSocket socket) throws IOException {
    }

    private void internalPrepareSocket(final SSLSocket socket) throws IOException {
        if (supportedProtocols != null) {
            socket.setEnabledProtocols(supportedProtocols);
        }
        if (supportedCipherSuites != null) {
            socket.setEnabledCipherSuites(supportedCipherSuites);
        }
        prepareSocket(socket);
    }

    public Socket createSocket(final HttpContext context) throws IOException {
        final SSLSocket sock = (SSLSocket) this.socketfactory.createSocket();
        internalPrepareSocket(sock);
        return sock;
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
        internalPrepareSocket(sslsock);
        sslsock.startHandshake();
        verifyHostname(sslsock, target);
        return sslsock;
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
