


























package ch.boye.httpclientandroidlib.impl.client;

import java.net.URI;
import java.net.URISyntaxException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.CircularRedirectException;
import ch.boye.httpclientandroidlib.client.RedirectHandler;
import ch.boye.httpclientandroidlib.client.methods.HttpGet;
import ch.boye.httpclientandroidlib.client.methods.HttpHead;
import ch.boye.httpclientandroidlib.client.params.ClientPNames;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;








@Immutable
@Deprecated
public class DefaultRedirectHandler implements RedirectHandler {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private static final String REDIRECT_LOCATIONS = "http.protocol.redirect-locations";

    public DefaultRedirectHandler() {
        super();
    }

    public boolean isRedirectRequested(
            final HttpResponse response,
            final HttpContext context) {
        Args.notNull(response, "HTTP response");

        final int statusCode = response.getStatusLine().getStatusCode();
        switch (statusCode) {
        case HttpStatus.SC_MOVED_TEMPORARILY:
        case HttpStatus.SC_MOVED_PERMANENTLY:
        case HttpStatus.SC_TEMPORARY_REDIRECT:
            final HttpRequest request = (HttpRequest) context.getAttribute(
                    ExecutionContext.HTTP_REQUEST);
            final String method = request.getRequestLine().getMethod();
            return method.equalsIgnoreCase(HttpGet.METHOD_NAME)
                || method.equalsIgnoreCase(HttpHead.METHOD_NAME);
        case HttpStatus.SC_SEE_OTHER:
            return true;
        default:
            return false;
        } 
    }

    public URI getLocationURI(
            final HttpResponse response,
            final HttpContext context) throws ProtocolException {
        Args.notNull(response, "HTTP response");
        
        final Header locationHeader = response.getFirstHeader("location");
        if (locationHeader == null) {
            
            throw new ProtocolException(
                    "Received redirect response " + response.getStatusLine()
                    + " but no location header");
        }
        final String location = locationHeader.getValue();
        if (this.log.isDebugEnabled()) {
            this.log.debug("Redirect requested to location '" + location + "'");
        }

        URI uri;
        try {
            uri = new URI(location);
        } catch (final URISyntaxException ex) {
            throw new ProtocolException("Invalid redirect URI: " + location, ex);
        }

        final HttpParams params = response.getParams();
        
        
        if (!uri.isAbsolute()) {
            if (params.isParameterTrue(ClientPNames.REJECT_RELATIVE_REDIRECT)) {
                throw new ProtocolException("Relative redirect location '"
                        + uri + "' not allowed");
            }
            
            final HttpHost target = (HttpHost) context.getAttribute(
                    ExecutionContext.HTTP_TARGET_HOST);
            Asserts.notNull(target, "Target host");

            final HttpRequest request = (HttpRequest) context.getAttribute(
                    ExecutionContext.HTTP_REQUEST);

            try {
                final URI requestURI = new URI(request.getRequestLine().getUri());
                final URI absoluteRequestURI = URIUtils.rewriteURI(requestURI, target, true);
                uri = URIUtils.resolve(absoluteRequestURI, uri);
            } catch (final URISyntaxException ex) {
                throw new ProtocolException(ex.getMessage(), ex);
            }
        }

        if (params.isParameterFalse(ClientPNames.ALLOW_CIRCULAR_REDIRECTS)) {

            RedirectLocations redirectLocations = (RedirectLocations) context.getAttribute(
                    REDIRECT_LOCATIONS);

            if (redirectLocations == null) {
                redirectLocations = new RedirectLocations();
                context.setAttribute(REDIRECT_LOCATIONS, redirectLocations);
            }

            final URI redirectURI;
            if (uri.getFragment() != null) {
                try {
                    final HttpHost target = new HttpHost(
                            uri.getHost(),
                            uri.getPort(),
                            uri.getScheme());
                    redirectURI = URIUtils.rewriteURI(uri, target, true);
                } catch (final URISyntaxException ex) {
                    throw new ProtocolException(ex.getMessage(), ex);
                }
            } else {
                redirectURI = uri;
            }

            if (redirectLocations.contains(redirectURI)) {
                throw new CircularRedirectException("Circular redirect to '" +
                        redirectURI + "'");
            } else {
                redirectLocations.add(redirectURI);
            }
        }

        return uri;
    }

}
