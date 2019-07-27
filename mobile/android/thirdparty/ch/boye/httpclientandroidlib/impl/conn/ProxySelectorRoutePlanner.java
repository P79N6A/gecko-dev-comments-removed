


























package ch.boye.httpclientandroidlib.impl.conn;


import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Proxy;
import java.net.ProxySelector;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.List;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.conn.params.ConnRouteParams;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoutePlanner;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.Asserts;























@NotThreadSafe 
@Deprecated
public class ProxySelectorRoutePlanner implements HttpRoutePlanner {

    
    protected final SchemeRegistry schemeRegistry; 

    
    protected ProxySelector proxySelector;

    






    public ProxySelectorRoutePlanner(final SchemeRegistry schreg,
                                     final ProxySelector prosel) {
        Args.notNull(schreg, "SchemeRegistry");
        schemeRegistry = schreg;
        proxySelector  = prosel;
    }

    




    public ProxySelector getProxySelector() {
        return this.proxySelector;
    }

    





    public void setProxySelector(final ProxySelector prosel) {
        this.proxySelector = prosel;
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
        final HttpHost proxy = determineProxy(target, request, context);

        final Scheme schm =
            this.schemeRegistry.getScheme(target.getSchemeName());
        
        
        final boolean secure = schm.isLayered();

        if (proxy == null) {
            route = new HttpRoute(target, local, secure);
        } else {
            route = new HttpRoute(target, local, proxy, secure);
        }
        return route;
    }

    











    protected HttpHost determineProxy(final HttpHost    target,
                                      final HttpRequest request,
                                      final HttpContext context)
        throws HttpException {

        
        ProxySelector psel = this.proxySelector;
        if (psel == null) {
            psel = ProxySelector.getDefault();
        }
        if (psel == null) {
            return null;
        }

        URI targetURI = null;
        try {
            targetURI = new URI(target.toURI());
        } catch (final URISyntaxException usx) {
            throw new HttpException
                ("Cannot convert host to URI: " + target, usx);
        }
        final List<Proxy> proxies = psel.select(targetURI);

        final Proxy p = chooseProxy(proxies, target, request, context);

        HttpHost result = null;
        if (p.type() == Proxy.Type.HTTP) {
            
            if (!(p.address() instanceof InetSocketAddress)) {
                throw new HttpException
                    ("Unable to handle non-Inet proxy address: "+p.address());
            }
            final InetSocketAddress isa = (InetSocketAddress) p.address();
            
            result = new HttpHost(getHost(isa), isa.getPort());
        }

        return result;
    }

    









    protected String getHost(final InetSocketAddress isa) {

        
        
        
       return isa.isUnresolved() ?
            isa.getHostName() : isa.getAddress().getHostAddress();

    }

    















    protected Proxy chooseProxy(final List<Proxy> proxies,
                                final HttpHost    target,
                                final HttpRequest request,
                                final HttpContext context) {
        Args.notEmpty(proxies, "List of proxies");

        Proxy result = null;

        
        for (int i=0; (result == null) && (i < proxies.size()); i++) {

            final Proxy p = proxies.get(i);
            switch (p.type()) {

            case DIRECT:
            case HTTP:
                result = p;
                break;

            case SOCKS:
                
                
                break;
            }
        }

        if (result == null) {
            
            
            
            result = Proxy.NO_PROXY;
        }

        return result;
    }

}

