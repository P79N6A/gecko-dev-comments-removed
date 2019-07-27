


























package ch.boye.httpclientandroidlib.impl;

import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpServerConnection;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.entity.ContentLengthStrategy;
import ch.boye.httpclientandroidlib.impl.entity.DisallowIdentityContentLengthStrategy;
import ch.boye.httpclientandroidlib.impl.io.DefaultHttpRequestParserFactory;
import ch.boye.httpclientandroidlib.impl.io.DefaultHttpResponseWriterFactory;
import ch.boye.httpclientandroidlib.io.HttpMessageParser;
import ch.boye.httpclientandroidlib.io.HttpMessageParserFactory;
import ch.boye.httpclientandroidlib.io.HttpMessageWriter;
import ch.boye.httpclientandroidlib.io.HttpMessageWriterFactory;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class DefaultBHttpServerConnection extends BHttpConnectionBase
                                                   implements HttpServerConnection {

    private final HttpMessageParser<HttpRequest> requestParser;
    private final HttpMessageWriter<HttpResponse> responseWriter;

    



















    public DefaultBHttpServerConnection(
            final int buffersize,
            final int fragmentSizeHint,
            final CharsetDecoder chardecoder,
            final CharsetEncoder charencoder,
            final MessageConstraints constraints,
            final ContentLengthStrategy incomingContentStrategy,
            final ContentLengthStrategy outgoingContentStrategy,
            final HttpMessageParserFactory<HttpRequest> requestParserFactory,
            final HttpMessageWriterFactory<HttpResponse> responseWriterFactory) {
        super(buffersize, fragmentSizeHint, chardecoder, charencoder, constraints,
                incomingContentStrategy != null ? incomingContentStrategy :
                    DisallowIdentityContentLengthStrategy.INSTANCE, outgoingContentStrategy);
        this.requestParser = (requestParserFactory != null ? requestParserFactory :
            DefaultHttpRequestParserFactory.INSTANCE).create(getSessionInputBuffer(), constraints);
        this.responseWriter = (responseWriterFactory != null ? responseWriterFactory :
            DefaultHttpResponseWriterFactory.INSTANCE).create(getSessionOutputBuffer());
    }

    public DefaultBHttpServerConnection(
            final int buffersize,
            final CharsetDecoder chardecoder,
            final CharsetEncoder charencoder,
            final MessageConstraints constraints) {
        this(buffersize, buffersize, chardecoder, charencoder, constraints, null, null, null, null);
    }

    public DefaultBHttpServerConnection(final int buffersize) {
        this(buffersize, buffersize, null, null, null, null, null, null, null);
    }

    protected void onRequestReceived(final HttpRequest request) {
    }

    protected void onResponseSubmitted(final HttpResponse response) {
    }

    @Override
    public void bind(final Socket socket) throws IOException {
        super.bind(socket);
    }

    public HttpRequest receiveRequestHeader()
            throws HttpException, IOException {
        ensureOpen();
        final HttpRequest request = this.requestParser.parse();
        onRequestReceived(request);
        incrementRequestCount();
        return request;
    }

    public void receiveRequestEntity(final HttpEntityEnclosingRequest request)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        ensureOpen();
        final HttpEntity entity = prepareInput(request);
        request.setEntity(entity);
    }

    public void sendResponseHeader(final HttpResponse response)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");
        ensureOpen();
        this.responseWriter.write(response);
        onResponseSubmitted(response);
        if (response.getStatusLine().getStatusCode() >= 200) {
            incrementResponseCount();
        }
    }

    public void sendResponseEntity(final HttpResponse response)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");
        ensureOpen();
        final HttpEntity entity = response.getEntity();
        if (entity == null) {
            return;
        }
        final OutputStream outstream = prepareOutput(response);
        entity.writeTo(outstream);
        outstream.close();
    }

    public void flush() throws IOException {
        ensureOpen();
        doFlush();
    }

}
