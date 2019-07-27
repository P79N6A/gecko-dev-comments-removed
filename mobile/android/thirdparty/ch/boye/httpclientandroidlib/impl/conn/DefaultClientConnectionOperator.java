


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.net.ConnectException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.protocol.ClientContext;
import ch.boye.httpclientandroidlib.conn.ClientConnectionOperator;
import ch.boye.httpclientandroidlib.conn.ConnectTimeoutException;
import ch.boye.httpclientandroidlib.conn.DnsResolver;
import ch.boye.httpclientandroidlib.conn.HttpInetSocketAddress;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeLayeredSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeSocketFactory;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;
































@Deprecated
@ThreadSafe
public class DefaultClientConnectionOperator implements ClientConnectionOperator {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    
    protected final SchemeRegistry schemeRegistry; 

    
    protected final DnsResolver dnsResolver;

    






    public DefaultClientConnectionOperator(final SchemeRegistry schemes) {
        Args.notNull(schemes, "Scheme registry");
        this.schemeRegistry = schemes;
        this.dnsResolver = new SystemDefaultDnsResolver();
    }

    








    public DefaultClientConnectionOperator(final SchemeRegistry schemes,final DnsResolver dnsResolver) {
        Args.notNull(schemes, "Scheme registry");

        Args.notNull(dnsResolver, "DNS resolver");

        this.schemeRegistry = schemes;
        this.dnsResolver = dnsResolver;
    }

    public OperatedClientConnection createConnection() {
        return new DefaultClientConnection();
    }

    private SchemeRegistry getSchemeRegistry(final HttpContext context) {
        SchemeRegistry reg = (SchemeRegistry) context.getAttribute(
                ClientContext.SCHEME_REGISTRY);
        if (reg == null) {
            reg = this.schemeRegistry;
        }
        return reg;
    }

    public void openConnection(
            final OperatedClientConnection conn,
            final HttpHost target,
            final InetAddress local,
            final HttpContext context,
            final HttpParams params) throws IOException {
        Args.notNull(conn, "Connection");
        Args.notNull(target, "Target host");
        Args.notNull(params, "HTTP parameters");
        Asserts.check(!conn.isOpen(), "Connection must not be open");

        final SchemeRegistry registry = getSchemeRegistry(context);
        final Scheme schm = registry.getScheme(target.getSchemeName());
        final SchemeSocketFactory sf = schm.getSchemeSocketFactory();

        final InetAddress[] addresses = resolveHostname(target.getHostName());
        final int port = schm.resolvePort(target.getPort());
        for (int i = 0; i < addresses.length; i++) {
            final InetAddress address = addresses[i];
            final boolean last = i == addresses.length - 1;

            Socket sock = sf.createSocket(params);
            conn.opening(sock, target);

            final InetSocketAddress remoteAddress = new HttpInetSocketAddress(target, address, port);
            InetSocketAddress localAddress = null;
            if (local != null) {
                localAddress = new InetSocketAddress(local, 0);
            }
            if (this.log.isDebugEnabled()) {
                this.log.debug("Connecting to " + remoteAddress);
            }
            try {
                final Socket connsock = sf.connectSocket(sock, remoteAddress, localAddress, params);
                if (sock != connsock) {
                    sock = connsock;
                    conn.opening(sock, target);
                }
                prepareSocket(sock, context, params);
                conn.openCompleted(sf.isSecure(sock), params);
                return;
            } catch (final ConnectException ex) {
                if (last) {
                    throw ex;
                }
            } catch (final ConnectTimeoutException ex) {
                if (last) {
                    throw ex;
                }
            }
            if (this.log.isDebugEnabled()) {
                this.log.debug("Connect to " + remoteAddress + " timed out. " +
                        "Connection will be retried using another IP address");
            }
        }
    }

    public void updateSecureConnection(
            final OperatedClientConnection conn,
            final HttpHost target,
            final HttpContext context,
            final HttpParams params) throws IOException {
        Args.notNull(conn, "Connection");
        Args.notNull(target, "Target host");
        Args.notNull(params, "Parameters");
        Asserts.check(conn.isOpen(), "Connection must be open");

        final SchemeRegistry registry = getSchemeRegistry(context);
        final Scheme schm = registry.getScheme(target.getSchemeName());
        Asserts.check(schm.getSchemeSocketFactory() instanceof SchemeLayeredSocketFactory,
            "Socket factory must implement SchemeLayeredSocketFactory");
        final SchemeLayeredSocketFactory lsf = (SchemeLayeredSocketFactory) schm.getSchemeSocketFactory();
        final Socket sock = lsf.createLayeredSocket(
                conn.getSocket(), target.getHostName(), schm.resolvePort(target.getPort()), params);
        prepareSocket(sock, context, params);
        conn.update(sock, target, lsf.isSecure(sock), params);
    }

    








    protected void prepareSocket(
            final Socket sock,
            final HttpContext context,
            final HttpParams params) throws IOException {
        sock.setTcpNoDelay(HttpConnectionParams.getTcpNoDelay(params));
        sock.setSoTimeout(HttpConnectionParams.getSoTimeout(params));

        final int linger = HttpConnectionParams.getLinger(params);
        if (linger >= 0) {
            sock.setSoLinger(linger > 0, linger);
        }
    }

    













    protected InetAddress[] resolveHostname(final String host) throws UnknownHostException {
            return dnsResolver.resolve(host);
    }

}

