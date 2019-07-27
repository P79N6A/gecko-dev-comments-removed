

























package ch.boye.httpclientandroidlib.pool;

import java.util.concurrent.Future;

import ch.boye.httpclientandroidlib.concurrent.FutureCallback;










public interface ConnPool<T, E> {

    












    Future<E> lease(final T route, final Object state, final FutureCallback<E> callback);

    






    void release(E entry, boolean reusable);

}
