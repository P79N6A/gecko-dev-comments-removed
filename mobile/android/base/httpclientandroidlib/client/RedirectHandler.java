


























package ch.boye.httpclientandroidlib.client;

import java.net.URI;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.protocol.HttpContext;














@Deprecated
public interface RedirectHandler {

    









    boolean isRedirectRequested(HttpResponse response, HttpContext context);

    









    URI getLocationURI(HttpResponse response, HttpContext context)
            throws ProtocolException;

}
