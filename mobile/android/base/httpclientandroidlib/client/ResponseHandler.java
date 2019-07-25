

























package ch.boye.httpclientandroidlib.client;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpResponse;








public interface ResponseHandler<T> {

    









    T handleResponse(HttpResponse response) throws ClientProtocolException, IOException;

}
