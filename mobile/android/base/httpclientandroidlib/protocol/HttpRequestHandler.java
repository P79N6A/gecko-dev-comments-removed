


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;











public interface HttpRequestHandler {

    










    void handle(HttpRequest request, HttpResponse response, HttpContext context)
            throws HttpException, IOException;

}
