


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;








@ThreadSafe
public class ResponseDate implements HttpResponseInterceptor {

    private static final HttpDateGenerator DATE_GENERATOR = new HttpDateGenerator();

    public ResponseDate() {
        super();
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");
        final int status = response.getStatusLine().getStatusCode();
        if ((status >= HttpStatus.SC_OK) &&
            !response.containsHeader(HTTP.DATE_HEADER)) {
            final String httpdate = DATE_GENERATOR.getCurrentDate();
            response.setHeader(HTTP.DATE_HEADER, httpdate);
        }
    }

}
