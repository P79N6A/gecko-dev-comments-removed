


























package ch.boye.httpclientandroidlib;

import java.io.IOException;

import ch.boye.httpclientandroidlib.protocol.HttpContext;


















public interface HttpRequestInterceptor {

    











    void process(HttpRequest request, HttpContext context)
        throws HttpException, IOException;

}
