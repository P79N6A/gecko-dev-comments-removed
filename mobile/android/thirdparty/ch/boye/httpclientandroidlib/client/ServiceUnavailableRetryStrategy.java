


























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.protocol.HttpContext;








public interface ServiceUnavailableRetryStrategy {

    










    boolean retryRequest(HttpResponse response, int executionCount, HttpContext context);

    


    long getRetryInterval();

}
