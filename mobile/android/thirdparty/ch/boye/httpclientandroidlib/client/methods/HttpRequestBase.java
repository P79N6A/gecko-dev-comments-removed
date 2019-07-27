


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.config.RequestConfig;
import ch.boye.httpclientandroidlib.message.BasicRequestLine;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;






@SuppressWarnings("deprecation")
@NotThreadSafe
public abstract class HttpRequestBase extends AbstractExecutionAwareRequest
    implements HttpUriRequest, Configurable {

    private ProtocolVersion version;
    private URI uri;
    private RequestConfig config;

    public abstract String getMethod();

    


    public void setProtocolVersion(final ProtocolVersion version) {
        this.version = version;
    }

    public ProtocolVersion getProtocolVersion() {
        return version != null ? version : HttpProtocolParams.getVersion(getParams());
    }

    





    public URI getURI() {
        return this.uri;
    }

    public RequestLine getRequestLine() {
        final String method = getMethod();
        final ProtocolVersion ver = getProtocolVersion();
        final URI uri = getURI();
        String uritext = null;
        if (uri != null) {
            uritext = uri.toASCIIString();
        }
        if (uritext == null || uritext.length() == 0) {
            uritext = "/";
        }
        return new BasicRequestLine(method, uritext, ver);
    }


    public RequestConfig getConfig() {
        return config;
    }

    public void setConfig(final RequestConfig config) {
        this.config = config;
    }

    public void setURI(final URI uri) {
        this.uri = uri;
    }

    


    public void started() {
    }

    





    public void releaseConnection() {
        reset();
    }

    @Override
    public String toString() {
        return getMethod() + " " + getURI() + " " + getProtocolVersion();
    }

}
