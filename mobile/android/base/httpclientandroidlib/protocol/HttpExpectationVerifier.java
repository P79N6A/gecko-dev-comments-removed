


























package ch.boye.httpclientandroidlib.protocol;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;



























public interface HttpExpectationVerifier {

    















    void verify(HttpRequest request, HttpResponse response, HttpContext context)
            throws HttpException;

}
