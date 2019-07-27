


























package ch.boye.httpclientandroidlib.impl;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;

import ch.boye.httpclientandroidlib.HttpInetConnection;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.impl.io.SocketInputBuffer;
import ch.boye.httpclientandroidlib.impl.io.SocketOutputBuffer;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.params.CoreConnectionPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;










@NotThreadSafe
@Deprecated
public class SocketHttpClientConnection
        extends AbstractHttpClientConnection implements HttpInetConnection {

    private volatile boolean open;
    private volatile Socket socket = null;

    public SocketHttpClientConnection() {
        super();
    }

    protected void assertNotOpen() {
        Asserts.check(!this.open, "Connection is already open");
    }

    @Override
    protected void assertOpen() {
        Asserts.check(this.open, "Connection is not open");
    }

    














    protected SessionInputBuffer createSessionInputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        return new SocketInputBuffer(socket, buffersize, params);
    }

    














    protected SessionOutputBuffer createSessionOutputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        return new SocketOutputBuffer(socket, buffersize, params);
    }

    

















    protected void bind(
            final Socket socket,
            final HttpParams params) throws IOException {
        Args.notNull(socket, "Socket");
        Args.notNull(params, "HTTP parameters");
        this.socket = socket;

        final int buffersize = params.getIntParameter(CoreConnectionPNames.SOCKET_BUFFER_SIZE, -1);
        init(
                createSessionInputBuffer(socket, buffersize, params),
                createSessionOutputBuffer(socket, buffersize, params),
                params);

        this.open = true;
    }

    public boolean isOpen() {
        return this.open;
    }

    protected Socket getSocket() {
        return this.socket;
    }

    public InetAddress getLocalAddress() {
        if (this.socket != null) {
            return this.socket.getLocalAddress();
        } else {
            return null;
        }
    }

    public int getLocalPort() {
        if (this.socket != null) {
            return this.socket.getLocalPort();
        } else {
            return -1;
        }
    }

    public InetAddress getRemoteAddress() {
        if (this.socket != null) {
            return this.socket.getInetAddress();
        } else {
            return null;
        }
    }

    public int getRemotePort() {
        if (this.socket != null) {
            return this.socket.getPort();
        } else {
            return -1;
        }
    }

    public void setSocketTimeout(final int timeout) {
        assertOpen();
        if (this.socket != null) {
            try {
                this.socket.setSoTimeout(timeout);
            } catch (final SocketException ignore) {
                
                
                
            }
        }
    }

    public int getSocketTimeout() {
        if (this.socket != null) {
            try {
                return this.socket.getSoTimeout();
            } catch (final SocketException ignore) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    public void shutdown() throws IOException {
        this.open = false;
        final Socket tmpsocket = this.socket;
        if (tmpsocket != null) {
            tmpsocket.close();
        }
    }

    public void close() throws IOException {
        if (!this.open) {
            return;
        }
        this.open = false;
        final Socket sock = this.socket;
        try {
            doFlush();
            try {
                try {
                    sock.shutdownOutput();
                } catch (final IOException ignore) {
                }
                try {
                    sock.shutdownInput();
                } catch (final IOException ignore) {
                }
            } catch (final UnsupportedOperationException ignore) {
                
            }
        } finally {
            sock.close();
        }
    }

    private static void formatAddress(final StringBuilder buffer, final SocketAddress socketAddress) {
        if (socketAddress instanceof InetSocketAddress) {
            final InetSocketAddress addr = ((InetSocketAddress) socketAddress);
            buffer.append(addr.getAddress() != null ? addr.getAddress().getHostAddress() :
                addr.getAddress())
            .append(':')
            .append(addr.getPort());
        } else {
            buffer.append(socketAddress);
        }
    }

    @Override
    public String toString() {
        if (this.socket != null) {
            final StringBuilder buffer = new StringBuilder();
            final SocketAddress remoteAddress = this.socket.getRemoteSocketAddress();
            final SocketAddress localAddress = this.socket.getLocalSocketAddress();
            if (remoteAddress != null && localAddress != null) {
                formatAddress(buffer, localAddress);
                buffer.append("<->");
                formatAddress(buffer, remoteAddress);
            }
            return buffer.toString();
        } else {
            return super.toString();
        }
    }

}
