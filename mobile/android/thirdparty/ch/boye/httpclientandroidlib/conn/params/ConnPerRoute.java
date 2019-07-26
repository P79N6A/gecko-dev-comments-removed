

























package ch.boye.httpclientandroidlib.conn.params;

import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;









public interface ConnPerRoute {

    int getMaxForRoute(HttpRoute route);

}
