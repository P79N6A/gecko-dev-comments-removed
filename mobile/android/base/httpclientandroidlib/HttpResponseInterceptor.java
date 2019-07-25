


























package ch.boye.httpclientandroidlib;

import java.io.IOException;

import ch.boye.httpclientandroidlib.protocol.HttpContext;


















public interface HttpResponseInterceptor {

    











    void process(HttpResponse response, HttpContext context)
        throws HttpException, IOException;

}
