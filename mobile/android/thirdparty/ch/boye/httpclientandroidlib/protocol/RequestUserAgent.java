


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;













public class RequestUserAgent implements HttpRequestInterceptor {

    public RequestUserAgent() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
        throws HttpException, IOException {
        if (request == null) {
            throw new IllegalArgumentException("HTTP request may not be null");
        }
        if (!request.containsHeader(HTTP.USER_AGENT)) {
            String useragent = HttpProtocolParams.getUserAgent(request.getParams());
            if (useragent != null) {
                request.addHeader(HTTP.USER_AGENT, useragent);
            }
        }
    }

}
