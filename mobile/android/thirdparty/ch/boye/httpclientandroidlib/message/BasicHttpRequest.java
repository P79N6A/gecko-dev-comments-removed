


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
public class BasicHttpRequest extends AbstractHttpMessage implements HttpRequest {

    private final String method;
    private final String uri;

    private RequestLine requestline;

    






    public BasicHttpRequest(final String method, final String uri) {
        super();
        this.method = Args.notNull(method, "Method name");
        this.uri = Args.notNull(uri, "Request URI");
        this.requestline = null;
    }

    







    public BasicHttpRequest(final String method, final String uri, final ProtocolVersion ver) {
        this(new BasicRequestLine(method, uri, ver));
    }

    




    public BasicHttpRequest(final RequestLine requestline) {
        super();
        this.requestline = Args.notNull(requestline, "Request line");
        this.method = requestline.getMethod();
        this.uri = requestline.getUri();
    }

    




    public ProtocolVersion getProtocolVersion() {
        return getRequestLine().getProtocolVersion();
    }

    




    public RequestLine getRequestLine() {
        if (this.requestline == null) {
            this.requestline = new BasicRequestLine(this.method, this.uri, HttpVersion.HTTP_1_1);
        }
        return this.requestline;
    }

    @Override
    public String toString() {
        return this.method + " " + this.uri + " " + this.headergroup;
    }

}
