


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.params.CoreProtocolPNames;













public class ResponseServer implements HttpResponseInterceptor {

    public ResponseServer() {
        super();
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        if (response == null) {
            throw new IllegalArgumentException("HTTP request may not be null");
        }
        if (!response.containsHeader(HTTP.SERVER_HEADER)) {
            String s = (String) response.getParams().getParameter(
                    CoreProtocolPNames.ORIGIN_SERVER);
            if (s != null) {
                response.addHeader(HTTP.SERVER_HEADER, s);
            }
        }
    }

}
