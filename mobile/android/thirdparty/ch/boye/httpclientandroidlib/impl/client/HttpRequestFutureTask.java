

























package ch.boye.httpclientandroidlib.impl.client;

import java.util.concurrent.FutureTask;

import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;







public class HttpRequestFutureTask<V> extends FutureTask<V> {

    private final HttpUriRequest request;
    private final HttpRequestTaskCallable<V> callable;

    public HttpRequestFutureTask(
            final HttpUriRequest request,
            final HttpRequestTaskCallable<V> httpCallable) {
        super(httpCallable);
        this.request = request;
        this.callable = httpCallable;
    }

    



    @Override
    public boolean cancel(final boolean mayInterruptIfRunning) {
        callable.cancel();
        if (mayInterruptIfRunning) {
            request.abort();
        }
        return super.cancel(mayInterruptIfRunning);
    }

    


    public long scheduledTime() {
        return callable.getScheduled();
    }

    


    public long startedTime() {
        return callable.getStarted();
    }

    


    public long endedTime() {
        if (isDone()) {
            return callable.getEnded();
        } else {
            throw new IllegalStateException("Task is not done yet");
        }
    }

    



    public long requestDuration() {
        if (isDone()) {
            return endedTime() - startedTime();
        } else {
            throw new IllegalStateException("Task is not done yet");
        }
    }

    


    public long taskDuration() {
        if (isDone()) {
            return endedTime() - scheduledTime();
        } else {
            throw new IllegalStateException("Task is not done yet");
        }
    }

    @Override
    public String toString() {
        return request.getRequestLine().getUri();
    }

}
