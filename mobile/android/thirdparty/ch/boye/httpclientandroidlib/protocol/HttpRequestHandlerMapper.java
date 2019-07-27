


























package ch.boye.httpclientandroidlib.protocol;

import ch.boye.httpclientandroidlib.HttpRequest;








public interface HttpRequestHandlerMapper {

    






    HttpRequestHandler lookup(HttpRequest request);

}
