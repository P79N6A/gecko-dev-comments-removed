

























package ch.boye.httpclientandroidlib.conn.params;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;











@Deprecated
@Immutable
public final class ConnManagerParams implements ConnManagerPNames {

    
    public static final int DEFAULT_MAX_TOTAL_CONNECTIONS = 20;

    









    @Deprecated
    public static long getTimeout(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getLongParameter(TIMEOUT, 0);
    }

    









    @Deprecated
    public static void setTimeout(final HttpParams params, final long timeout) {
        Args.notNull(params, "HTTP parameters");
        params.setLongParameter(TIMEOUT, timeout);
    }

    
    private static final ConnPerRoute DEFAULT_CONN_PER_ROUTE = new ConnPerRoute() {

        public int getMaxForRoute(final HttpRoute route) {
            return ConnPerRouteBean.DEFAULT_MAX_CONNECTIONS_PER_ROUTE;
        }

    };

    






    public static void setMaxConnectionsPerRoute(final HttpParams params,
                                                final ConnPerRoute connPerRoute) {
        Args.notNull(params, "HTTP parameters");
        params.setParameter(MAX_CONNECTIONS_PER_ROUTE, connPerRoute);
    }

    






    public static ConnPerRoute getMaxConnectionsPerRoute(final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        ConnPerRoute connPerRoute = (ConnPerRoute) params.getParameter(MAX_CONNECTIONS_PER_ROUTE);
        if (connPerRoute == null) {
            connPerRoute = DEFAULT_CONN_PER_ROUTE;
        }
        return connPerRoute;
    }

    





    public static void setMaxTotalConnections(
            final HttpParams params,
            final int maxTotalConnections) {
        Args.notNull(params, "HTTP parameters");
        params.setIntParameter(MAX_TOTAL_CONNECTIONS, maxTotalConnections);
    }

    






    public static int getMaxTotalConnections(
            final HttpParams params) {
        Args.notNull(params, "HTTP parameters");
        return params.getIntParameter(MAX_TOTAL_CONNECTIONS, DEFAULT_MAX_TOTAL_CONNECTIONS);
    }

}
