

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.InetAddress;
import java.net.Socket;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import ch.boye.httpclientandroidlib.HttpConnectionMetrics;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
























@Deprecated
@NotThreadSafe
public abstract class AbstractClientConnAdapter implements ManagedClientConnection, HttpContext {

    


    private final ClientConnectionManager connManager;

    
    private volatile OperatedClientConnection wrappedConnection;

    
    private volatile boolean markedReusable;

    
    private volatile boolean released;

    
    private volatile long duration;

    







    protected AbstractClientConnAdapter(final ClientConnectionManager mgr,
                                        final OperatedClientConnection conn) {
        super();
        connManager = mgr;
        wrappedConnection = conn;
        markedReusable = false;
        released = false;
        duration = Long.MAX_VALUE;
    }

    



    protected synchronized void detach() {
        wrappedConnection = null;
        duration = Long.MAX_VALUE;
    }

    protected OperatedClientConnection getWrappedConnection() {
        return wrappedConnection;
    }

    protected ClientConnectionManager getManager() {
        return connManager;
    }

    


    @Deprecated
    protected final void assertNotAborted() throws InterruptedIOException {
        if (isReleased()) {
            throw new InterruptedIOException("Connection has been shut down");
        }
    }

    



    protected boolean isReleased() {
        return released;
    }

    





    protected final void assertValid(
            final OperatedClientConnection wrappedConn) throws ConnectionShutdownException {
        if (isReleased() || wrappedConn == null) {
            throw new ConnectionShutdownException();
        }
    }

    public boolean isOpen() {
        final OperatedClientConnection conn = getWrappedConnection();
        if (conn == null) {
            return false;
        }

        return conn.isOpen();
    }

    public boolean isStale() {
        if (isReleased()) {
            return true;
        }
        final OperatedClientConnection conn = getWrappedConnection();
        if (conn == null) {
            return true;
        }

        return conn.isStale();
    }

    public void setSocketTimeout(final int timeout) {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        conn.setSocketTimeout(timeout);
    }

    public int getSocketTimeout() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getSocketTimeout();
    }

    public HttpConnectionMetrics getMetrics() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getMetrics();
    }

    public void flush() throws IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        conn.flush();
    }

    public boolean isResponseAvailable(final int timeout) throws IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.isResponseAvailable(timeout);
    }

    public void receiveResponseEntity(final HttpResponse response)
        throws HttpException, IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        unmarkReusable();
        conn.receiveResponseEntity(response);
    }

    public HttpResponse receiveResponseHeader()
        throws HttpException, IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        unmarkReusable();
        return conn.receiveResponseHeader();
    }

    public void sendRequestEntity(final HttpEntityEnclosingRequest request)
        throws HttpException, IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        unmarkReusable();
        conn.sendRequestEntity(request);
    }

    public void sendRequestHeader(final HttpRequest request)
        throws HttpException, IOException {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        unmarkReusable();
        conn.sendRequestHeader(request);
    }

    public InetAddress getLocalAddress() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getLocalAddress();
    }

    public int getLocalPort() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getLocalPort();
    }

    public InetAddress getRemoteAddress() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getRemoteAddress();
    }

    public int getRemotePort() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.getRemotePort();
    }

    public boolean isSecure() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        return conn.isSecure();
    }

    public void bind(final Socket socket) throws IOException {
        throw new UnsupportedOperationException();
    }

    public Socket getSocket() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        if (!isOpen()) {
            return null;
        }
        return conn.getSocket();
    }

    public SSLSession getSSLSession() {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        if (!isOpen()) {
            return null;
        }

        SSLSession result = null;
        final Socket    sock    = conn.getSocket();
        if (sock instanceof SSLSocket) {
            result = ((SSLSocket)sock).getSession();
        }
        return result;
    }

    public void markReusable() {
        markedReusable = true;
    }

    public void unmarkReusable() {
        markedReusable = false;
    }

    public boolean isMarkedReusable() {
        return markedReusable;
    }

    public void setIdleDuration(final long duration, final TimeUnit unit) {
        if(duration > 0) {
            this.duration = unit.toMillis(duration);
        } else {
            this.duration = -1;
        }
    }

    public synchronized void releaseConnection() {
        if (released) {
            return;
        }
        released = true;
        connManager.releaseConnection(this, duration, TimeUnit.MILLISECONDS);
    }

    public synchronized void abortConnection() {
        if (released) {
            return;
        }
        released = true;
        unmarkReusable();
        try {
            shutdown();
        } catch (final IOException ignore) {
        }
        connManager.releaseConnection(this, duration, TimeUnit.MILLISECONDS);
    }

    public Object getAttribute(final String id) {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        if (conn instanceof HttpContext) {
            return ((HttpContext) conn).getAttribute(id);
        } else {
            return null;
        }
    }

    public Object removeAttribute(final String id) {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        if (conn instanceof HttpContext) {
            return ((HttpContext) conn).removeAttribute(id);
        } else {
            return null;
        }
    }

    public void setAttribute(final String id, final Object obj) {
        final OperatedClientConnection conn = getWrappedConnection();
        assertValid(conn);
        if (conn instanceof HttpContext) {
            ((HttpContext) conn).setAttribute(id, obj);
        }
    }

}
