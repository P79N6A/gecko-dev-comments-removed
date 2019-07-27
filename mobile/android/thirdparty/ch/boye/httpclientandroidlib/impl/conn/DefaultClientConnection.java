


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.Socket;
import java.util.HashMap;
import java.util.Map;

import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.conn.OperatedClientConnection;
import ch.boye.httpclientandroidlib.conn.ManagedHttpClientConnection;
import ch.boye.httpclientandroidlib.impl.SocketHttpClientConnection;
import ch.boye.httpclientandroidlib.io.HttpMessageParser;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.params.BasicHttpParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;








@NotThreadSafe 
@Deprecated
public class DefaultClientConnection extends SocketHttpClientConnection
    implements OperatedClientConnection, ManagedHttpClientConnection, HttpContext {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());
    public HttpClientAndroidLog headerLog = new HttpClientAndroidLog("ch.boye.httpclientandroidlib.headers");
    public HttpClientAndroidLog wireLog = new HttpClientAndroidLog("ch.boye.httpclientandroidlib.wire");

    
    private volatile Socket socket;

    
    private HttpHost targetHost;

    
    private boolean connSecure;

    
    private volatile boolean shutdown;

    
    private final Map<String, Object> attributes;

    public DefaultClientConnection() {
        super();
        this.attributes = new HashMap<String, Object>();
    }

    public String getId() {
        return null;
    }

    public final HttpHost getTargetHost() {
        return this.targetHost;
    }

    public final boolean isSecure() {
        return this.connSecure;
    }

    @Override
    public final Socket getSocket() {
        return this.socket;
    }

    public SSLSession getSSLSession() {
        if (this.socket instanceof SSLSocket) {
            return ((SSLSocket) this.socket).getSession();
        } else {
            return null;
        }
    }

    public void opening(final Socket sock, final HttpHost target) throws IOException {
        assertNotOpen();
        this.socket = sock;
        this.targetHost = target;

        
        if (this.shutdown) {
            sock.close(); 
            
            throw new InterruptedIOException("Connection already shutdown");
        }
    }

    public void openCompleted(final boolean secure, final HttpParams params) throws IOException {
        Args.notNull(params, "Parameters");
        assertNotOpen();
        this.connSecure = secure;
        bind(this.socket, params);
    }

    












    @Override
    public void shutdown() throws IOException {
        shutdown = true;
        try {
            super.shutdown();
            if (log.isDebugEnabled()) {
                log.debug("Connection " + this + " shut down");
            }
            final Socket sock = this.socket; 
            if (sock != null) {
                sock.close();
            }
        } catch (final IOException ex) {
            log.debug("I/O error shutting down connection", ex);
        }
    }

    @Override
    public void close() throws IOException {
        try {
            super.close();
            if (log.isDebugEnabled()) {
                log.debug("Connection " + this + " closed");
            }
        } catch (final IOException ex) {
            log.debug("I/O error closing connection", ex);
        }
    }

    @Override
    protected SessionInputBuffer createSessionInputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        SessionInputBuffer inbuffer = super.createSessionInputBuffer(
                socket,
                buffersize > 0 ? buffersize : 8192,
                params);
        if (wireLog.isDebugEnabled()) {
            inbuffer = new LoggingSessionInputBuffer(
                    inbuffer,
                    new Wire(wireLog),
                    HttpProtocolParams.getHttpElementCharset(params));
        }
        return inbuffer;
    }

    @Override
    protected SessionOutputBuffer createSessionOutputBuffer(
            final Socket socket,
            final int buffersize,
            final HttpParams params) throws IOException {
        SessionOutputBuffer outbuffer = super.createSessionOutputBuffer(
                socket,
                buffersize > 0 ? buffersize : 8192,
                params);
        if (wireLog.isDebugEnabled()) {
            outbuffer = new LoggingSessionOutputBuffer(
                    outbuffer,
                    new Wire(wireLog),
                    HttpProtocolParams.getHttpElementCharset(params));
        }
        return outbuffer;
    }

    @Override
    protected HttpMessageParser<HttpResponse> createResponseParser(
            final SessionInputBuffer buffer,
            final HttpResponseFactory responseFactory,
            final HttpParams params) {
        
        return new DefaultHttpResponseParser
            (buffer, null, responseFactory, params);
    }

    public void bind(final Socket socket) throws IOException {
        bind(socket, new BasicHttpParams());
    }

    public void update(final Socket sock, final HttpHost target,
                       final boolean secure, final HttpParams params)
        throws IOException {

        assertOpen();
        Args.notNull(target, "Target host");
        Args.notNull(params, "Parameters");

        if (sock != null) {
            this.socket = sock;
            bind(sock, params);
        }
        targetHost = target;
        connSecure = secure;
    }

    @Override
    public HttpResponse receiveResponseHeader() throws HttpException, IOException {
        final HttpResponse response = super.receiveResponseHeader();
        if (log.isDebugEnabled()) {
            log.debug("Receiving response: " + response.getStatusLine());
        }
        if (headerLog.isDebugEnabled()) {
            headerLog.debug("<< " + response.getStatusLine().toString());
            final Header[] headers = response.getAllHeaders();
            for (final Header header : headers) {
                headerLog.debug("<< " + header.toString());
            }
        }
        return response;
    }

    @Override
    public void sendRequestHeader(final HttpRequest request) throws HttpException, IOException {
        if (log.isDebugEnabled()) {
            log.debug("Sending request: " + request.getRequestLine());
        }
        super.sendRequestHeader(request);
        if (headerLog.isDebugEnabled()) {
            headerLog.debug(">> " + request.getRequestLine().toString());
            final Header[] headers = request.getAllHeaders();
            for (final Header header : headers) {
                headerLog.debug(">> " + header.toString());
            }
        }
    }

    public Object getAttribute(final String id) {
        return this.attributes.get(id);
    }

    public Object removeAttribute(final String id) {
        return this.attributes.remove(id);
    }

    public void setAttribute(final String id, final Object obj) {
        this.attributes.put(id, obj);
    }

}
