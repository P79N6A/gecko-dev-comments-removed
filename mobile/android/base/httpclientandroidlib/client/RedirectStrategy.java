


























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.protocol.HttpContext;












public interface RedirectStrategy {

    










    boolean isRedirected(
            HttpRequest request,
            HttpResponse response,
            HttpContext context) throws ProtocolException;

    










    HttpUriRequest getRedirect(
            HttpRequest request,
            HttpResponse response,
            HttpContext context) throws ProtocolException;

}