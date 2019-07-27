

























package ch.boye.httpclientandroidlib.client;

import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;









public interface BackoffManager {

    




    public void backOff(HttpRoute route);

    




    public void probe(HttpRoute route);
}
