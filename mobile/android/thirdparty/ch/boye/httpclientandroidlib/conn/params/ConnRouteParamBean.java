


























package ch.boye.httpclientandroidlib.conn.params;

import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpAbstractParamBean;
import ch.boye.httpclientandroidlib.params.HttpParams;










@Deprecated
@NotThreadSafe
public class ConnRouteParamBean extends HttpAbstractParamBean {

    public ConnRouteParamBean (final HttpParams params) {
        super(params);
    }

    
    public void setDefaultProxy (final HttpHost defaultProxy) {
        params.setParameter(ConnRoutePNames.DEFAULT_PROXY, defaultProxy);
    }

    
    public void setLocalAddress (final InetAddress address) {
        params.setParameter(ConnRoutePNames.LOCAL_ADDRESS, address);
    }

    
    public void setForcedRoute (final HttpRoute route) {
        params.setParameter(ConnRoutePNames.FORCED_ROUTE, route);
    }

}
