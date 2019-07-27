

























package ch.boye.httpclientandroidlib.conn.params;

import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;









@Deprecated
@Immutable
public class ConnRouteParams implements ConnRoutePNames {

    




    public static final HttpHost NO_HOST =
        new HttpHost("127.0.0.255", 0, "no-host"); 

    



    public static final HttpRoute NO_ROUTE = new HttpRoute(NO_HOST); 

    
    private ConnRouteParams() {
        
    }

    










    public static HttpHost getDefaultProxy(final HttpParams params) {
        Args.notNull(params, "Parameters");
        HttpHost proxy = (HttpHost)
            params.getParameter(DEFAULT_PROXY);
        if ((proxy != null) && NO_HOST.equals(proxy)) {
            
            proxy = null;
        }
        return proxy;
    }

    









    public static void setDefaultProxy(final HttpParams params,
                                             final HttpHost proxy) {
        Args.notNull(params, "Parameters");
        params.setParameter(DEFAULT_PROXY, proxy);
    }

    










    public static HttpRoute getForcedRoute(final HttpParams params) {
        Args.notNull(params, "Parameters");
        HttpRoute route = (HttpRoute)
            params.getParameter(FORCED_ROUTE);
        if ((route != null) && NO_ROUTE.equals(route)) {
            
            route = null;
        }
        return route;
    }

    









    public static void setForcedRoute(final HttpParams params,
                                            final HttpRoute route) {
        Args.notNull(params, "Parameters");
        params.setParameter(FORCED_ROUTE, route);
    }

    











    public static InetAddress getLocalAddress(final HttpParams params) {
        Args.notNull(params, "Parameters");
        final InetAddress local = (InetAddress)
            params.getParameter(LOCAL_ADDRESS);
        
        return local;
    }

    






    public static void setLocalAddress(final HttpParams params,
                                             final InetAddress local) {
        Args.notNull(params, "Parameters");
        params.setParameter(LOCAL_ADDRESS, local);
    }

}

