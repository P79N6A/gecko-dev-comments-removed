

























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.protocol.HttpContext;










@Immutable
public class RequestAcceptEncoding implements HttpRequestInterceptor {

    


    public void process(
            final HttpRequest request,
            final HttpContext context) throws HttpException, IOException {

        
        if (!request.containsHeader("Accept-Encoding")) {
            request.addHeader("Accept-Encoding", "gzip,deflate");
        }
    }

}
