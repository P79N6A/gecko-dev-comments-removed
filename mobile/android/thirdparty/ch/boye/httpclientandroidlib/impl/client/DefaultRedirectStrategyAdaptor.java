


























package ch.boye.httpclientandroidlib.impl.client;

import java.net.URI;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.RedirectHandler;
import ch.boye.httpclientandroidlib.client.RedirectStrategy;
import ch.boye.httpclientandroidlib.client.methods.HttpGet;
import ch.boye.httpclientandroidlib.client.methods.HttpHead;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.protocol.HttpContext;




@Immutable
@Deprecated
class DefaultRedirectStrategyAdaptor implements RedirectStrategy {

    private final RedirectHandler handler;

    public DefaultRedirectStrategyAdaptor(final RedirectHandler handler) {
        super();
        this.handler = handler;
    }

    public boolean isRedirected(
            final HttpRequest request,
            final HttpResponse response,
            final HttpContext context) throws ProtocolException {
        return this.handler.isRedirectRequested(response, context);
    }

    public HttpUriRequest getRedirect(
            final HttpRequest request,
            final HttpResponse response,
            final HttpContext context) throws ProtocolException {
        final URI uri = this.handler.getLocationURI(response, context);
        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase(HttpHead.METHOD_NAME)) {
            return new HttpHead(uri);
        } else {
            return new HttpGet(uri);
        }
    }

    public RedirectHandler getHandler() {
        return this.handler;
    }

}
