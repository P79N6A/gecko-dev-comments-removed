

























package ch.boye.httpclientandroidlib.impl.client;

import java.util.HashMap;
import java.util.Map;

import ch.boye.httpclientandroidlib.client.BackoffManager;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;
import ch.boye.httpclientandroidlib.pool.ConnPoolControl;
import ch.boye.httpclientandroidlib.util.Args;





















public class AIMDBackoffManager implements BackoffManager {

    private final ConnPoolControl<HttpRoute> connPerRoute;
    private final Clock clock;
    private final Map<HttpRoute,Long> lastRouteProbes;
    private final Map<HttpRoute,Long> lastRouteBackoffs;
    private long coolDown = 5 * 1000L;
    private double backoffFactor = 0.5;
    private int cap = 2; 

    






    public AIMDBackoffManager(final ConnPoolControl<HttpRoute> connPerRoute) {
        this(connPerRoute, new SystemClock());
    }

    AIMDBackoffManager(final ConnPoolControl<HttpRoute> connPerRoute, final Clock clock) {
        this.clock = clock;
        this.connPerRoute = connPerRoute;
        this.lastRouteProbes = new HashMap<HttpRoute,Long>();
        this.lastRouteBackoffs = new HashMap<HttpRoute,Long>();
    }

    public void backOff(final HttpRoute route) {
        synchronized(connPerRoute) {
            final int curr = connPerRoute.getMaxPerRoute(route);
            final Long lastUpdate = getLastUpdate(lastRouteBackoffs, route);
            final long now = clock.getCurrentTime();
            if (now - lastUpdate.longValue() < coolDown) {
                return;
            }
            connPerRoute.setMaxPerRoute(route, getBackedOffPoolSize(curr));
            lastRouteBackoffs.put(route, Long.valueOf(now));
        }
    }

    private int getBackedOffPoolSize(final int curr) {
        if (curr <= 1) {
            return 1;
        }
        return (int)(Math.floor(backoffFactor * curr));
    }

    public void probe(final HttpRoute route) {
        synchronized(connPerRoute) {
            final int curr = connPerRoute.getMaxPerRoute(route);
            final int max = (curr >= cap) ? cap : curr + 1;
            final Long lastProbe = getLastUpdate(lastRouteProbes, route);
            final Long lastBackoff = getLastUpdate(lastRouteBackoffs, route);
            final long now = clock.getCurrentTime();
            if (now - lastProbe.longValue() < coolDown || now - lastBackoff.longValue() < coolDown) {
                return;
            }
            connPerRoute.setMaxPerRoute(route, max);
            lastRouteProbes.put(route, Long.valueOf(now));
        }
    }

    private Long getLastUpdate(final Map<HttpRoute,Long> updates, final HttpRoute route) {
        Long lastUpdate = updates.get(route);
        if (lastUpdate == null) {
            lastUpdate = Long.valueOf(0L);
        }
        return lastUpdate;
    }

    








    public void setBackoffFactor(final double d) {
        Args.check(d > 0.0 && d < 1.0, "Backoff factor must be 0.0 < f < 1.0");
        backoffFactor = d;
    }

    






    public void setCooldownMillis(final long l) {
        Args.positive(coolDown, "Cool down");
        coolDown = l;
    }

    




    public void setPerHostConnectionCap(final int cap) {
        Args.positive(cap, "Per host connection cap");
        this.cap = cap;
    }

}
