


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.params.CoreProtocolPNames;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;







@SuppressWarnings("deprecation")
@Immutable
public class RequestUserAgent implements HttpRequestInterceptor {

    private final String userAgent;

    public RequestUserAgent(final String userAgent) {
        super();
        this.userAgent = userAgent;
    }

    public RequestUserAgent() {
        this(null);
    }

    public void process(final HttpRequest request, final HttpContext context)
        throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        if (!request.containsHeader(HTTP.USER_AGENT)) {
            String s = null;
            final HttpParams params = request.getParams();
            if (params != null) {
                s = (String) params.getParameter(CoreProtocolPNames.USER_AGENT);
            }
            if (s == null) {
                s = this.userAgent;
            }
            if (s != null) {
                request.addHeader(HTTP.USER_AGENT, s);
            }
        }
    }

}
