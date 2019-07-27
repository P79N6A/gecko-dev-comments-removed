


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;








@ThreadSafe
public class RequestDate implements HttpRequestInterceptor {

    private static final HttpDateGenerator DATE_GENERATOR = new HttpDateGenerator();

    public RequestDate() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        if ((request instanceof HttpEntityEnclosingRequest) &&
            !request.containsHeader(HTTP.DATE_HEADER)) {
            final String httpdate = DATE_GENERATOR.getCurrentDate();
            request.setHeader(HTTP.DATE_HEADER, httpdate);
        }
    }

}
