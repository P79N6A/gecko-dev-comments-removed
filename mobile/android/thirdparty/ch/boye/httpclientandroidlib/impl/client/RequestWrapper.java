


























package ch.boye.httpclientandroidlib.impl.client;

import java.net.URI;
import java.net.URISyntaxException;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.message.AbstractHttpMessage;
import ch.boye.httpclientandroidlib.message.BasicRequestLine;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.util.Args;













@NotThreadSafe
@Deprecated
public class RequestWrapper extends AbstractHttpMessage implements HttpUriRequest {

    private final HttpRequest original;

    private URI uri;
    private String method;
    private ProtocolVersion version;
    private int execCount;

    public RequestWrapper(final HttpRequest request) throws ProtocolException {
        super();
        Args.notNull(request, "HTTP request");
        this.original = request;
        setParams(request.getParams());
        setHeaders(request.getAllHeaders());
        
        if (request instanceof HttpUriRequest) {
            this.uri = ((HttpUriRequest) request).getURI();
            this.method = ((HttpUriRequest) request).getMethod();
            this.version = null;
        } else {
            final RequestLine requestLine = request.getRequestLine();
            try {
                this.uri = new URI(requestLine.getUri());
            } catch (final URISyntaxException ex) {
                throw new ProtocolException("Invalid request URI: "
                        + requestLine.getUri(), ex);
            }
            this.method = requestLine.getMethod();
            this.version = request.getProtocolVersion();
        }
        this.execCount = 0;
    }

    public void resetHeaders() {
        
        this.headergroup.clear();
        setHeaders(this.original.getAllHeaders());
    }

    public String getMethod() {
        return this.method;
    }

    public void setMethod(final String method) {
        Args.notNull(method, "Method name");
        this.method = method;
    }

    public ProtocolVersion getProtocolVersion() {
        if (this.version == null) {
            this.version = HttpProtocolParams.getVersion(getParams());
        }
        return this.version;
    }

    public void setProtocolVersion(final ProtocolVersion version) {
        this.version = version;
    }


    public URI getURI() {
        return this.uri;
    }

    public void setURI(final URI uri) {
        this.uri = uri;
    }

    public RequestLine getRequestLine() {
        final String method = getMethod();
        final ProtocolVersion ver = getProtocolVersion();
        String uritext = null;
        if (uri != null) {
            uritext = uri.toASCIIString();
        }
        if (uritext == null || uritext.length() == 0) {
            uritext = "/";
        }
        return new BasicRequestLine(method, uritext, ver);
    }

    public void abort() throws UnsupportedOperationException {
        throw new UnsupportedOperationException();
    }

    public boolean isAborted() {
        return false;
    }

    public HttpRequest getOriginal() {
        return this.original;
    }

    public boolean isRepeatable() {
        return true;
    }

    public int getExecCount() {
        return this.execCount;
    }

    public void incrementExecCount() {
        this.execCount++;
    }

}
