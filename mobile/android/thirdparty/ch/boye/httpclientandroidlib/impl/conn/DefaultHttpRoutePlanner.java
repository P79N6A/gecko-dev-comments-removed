


























package ch.boye.httpclientandroidlib.impl.conn;


import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.params.ConnRouteParams;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;



















@ThreadSafe
@Deprecated
public class DefaultHttpRoutePlanner implements HttpRoutePlanner {

    
    protected final SchemeRegistry schemeRegistry; 

    




    public DefaultHttpRoutePlanner(final SchemeRegistry schreg) {
        Args.notNull(schreg, "Scheme registry");
        schemeRegistry = schreg;
    }

    public HttpRoute determineRoute(final HttpHost target,
                                    final HttpRequest request,
                                    final HttpContext context)
        throws HttpException {

        Args.notNull(request, "HTTP request");

        
        HttpRoute route =
            ConnRouteParams.getForcedRoute(request.getParams());
        if (route != null) {
            return route;
        }

        
        

        Asserts.notNull(target, "Target host");

        final InetAddress local =
            ConnRouteParams.getLocalAddress(request.getParams());
        final HttpHost proxy =
            ConnRouteParams.getDefaultProxy(request.getParams());

        final Scheme schm;
        try {
            schm = this.schemeRegistry.getScheme(target.getSchemeName());
        } catch (final IllegalStateException ex) {
            throw new HttpException(ex.getMessage());
        }
        
        
        final boolean secure = schm.isLayered();

        if (proxy == null) {
            route = new HttpRoute(target, local, secure);
        } else {
            route = new HttpRoute(target, local, proxy, secure);
        }
        return route;
    }

}
