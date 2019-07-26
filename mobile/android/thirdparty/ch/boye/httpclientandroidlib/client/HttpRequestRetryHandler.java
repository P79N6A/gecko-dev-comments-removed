


























package ch.boye.httpclientandroidlib.client;

import java.io.IOException;

import ch.boye.httpclientandroidlib.protocol.HttpContext;











public interface HttpRequestRetryHandler {

    











    boolean retryRequest(IOException exception, int executionCount, HttpContext context);

}
