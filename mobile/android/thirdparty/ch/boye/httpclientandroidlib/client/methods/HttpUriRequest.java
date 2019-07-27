


























package ch.boye.httpclientandroidlib.client.methods;

import java.net.URI;

import ch.boye.httpclientandroidlib.HttpRequest;








public interface HttpUriRequest extends HttpRequest {

    



    String getMethod();

    
















    URI getURI();

    





    void abort() throws UnsupportedOperationException;

    





    boolean isAborted();

}
