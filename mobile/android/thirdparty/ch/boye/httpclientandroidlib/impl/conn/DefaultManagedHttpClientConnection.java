


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.io.InterruptedIOException;
import java.net.Socket;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import javax.net.ssl.SSLSession;
import javax.net.ssl.SSLSocket;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.conn.ManagedHttpClientConnection;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.impl.DefaultBHttpClientConnection;
import ch.boye.httpclientandroidlib.io.HttpMessageParserFactory;
import ch.boye.httpclientandroidlib.io.HttpMessageWriterFactory;
import ch.boye.httpclientandroidlib.protocol.HttpContext;





@NotThreadSafe
public class DefaultManagedHttpClientConnection extends DefaultBHttpClientConnection
                                 implements ManagedHttpClientConnection, HttpContext {

    private final String id;
    private final Map<String, Object> attributes;

    private volatile boolean shutdown;

    public DefaultManagedHttpClientConnection(
            final String id,
            final int buffersize,
            final int fragmentSizeHint,
            final CharsetDecoder chardecoder,
            final CharsetEncoder charencoder,
            final MessageConstraints constraints,
            final ContentLengthStrategy incomingContentStrategy,
            final ContentLengthStrategy outgoingContentStrategy,
            final HttpMessageWriterFactory<HttpRequest> requestWriterFactory,
            final HttpMessageParserFactory<HttpResponse> responseParserFactory) {
        super(buffersize, fragmentSizeHint, chardecoder, charencoder,
                constraints, incomingContentStrategy, outgoingContentStrategy,
                requestWriterFactory, responseParserFactory);
        this.id = id;
        this.attributes = new ConcurrentHashMap<String, Object>();
    }

    public DefaultManagedHttpClientConnection(
            final String id,
            final int buffersize) {
        this(id, buffersize, buffersize, null, null, null, null, null, null, null);
    }

    public String getId() {
        return this.id;
    }

    @Override
    public void shutdown() throws IOException {
        this.shutdown = true;
        super.shutdown();
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

    @Override
    public void bind(final Socket socket) throws IOException {
        if (this.shutdown) {
            socket.close(); 
            
            throw new InterruptedIOException("Connection already shutdown");
        }
        super.bind(socket);
    }

    @Override
    public Socket getSocket() {
        return super.getSocket();
    }

    public SSLSession getSSLSession() {
        final Socket socket = super.getSocket();
        if (socket instanceof SSLSocket) {
            return ((SSLSocket) socket).getSession();
        } else {
            return null;
        }
    }

}
