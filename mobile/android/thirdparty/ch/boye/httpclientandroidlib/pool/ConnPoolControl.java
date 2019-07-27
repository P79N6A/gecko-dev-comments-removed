

























package ch.boye.httpclientandroidlib.pool;










public interface ConnPoolControl<T> {

    void setMaxTotal(int max);

    int getMaxTotal();

    void setDefaultMaxPerRoute(int max);

    int getDefaultMaxPerRoute();

    void setMaxPerRoute(final T route, int max);

    int getMaxPerRoute(final T route);

    PoolStats getTotalStats();

    PoolStats getStats(final T route);

}
