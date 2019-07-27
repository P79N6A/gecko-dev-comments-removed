

























package ch.boye.httpclientandroidlib.concurrent;

import ch.boye.httpclientandroidlib.util.Args;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;









public class BasicFuture<T> implements Future<T>, Cancellable {

    private final FutureCallback<T> callback;

    private volatile boolean completed;
    private volatile boolean cancelled;
    private volatile T result;
    private volatile Exception ex;

    public BasicFuture(final FutureCallback<T> callback) {
        super();
        this.callback = callback;
    }

    public boolean isCancelled() {
        return this.cancelled;
    }

    public boolean isDone() {
        return this.completed;
    }

    private T getResult() throws ExecutionException {
        if (this.ex != null) {
            throw new ExecutionException(this.ex);
        }
        return this.result;
    }

    public synchronized T get() throws InterruptedException, ExecutionException {
        while (!this.completed) {
            wait();
        }
        return getResult();
    }

    public synchronized T get(final long timeout, final TimeUnit unit)
            throws InterruptedException, ExecutionException, TimeoutException {
        Args.notNull(unit, "Time unit");
        final long msecs = unit.toMillis(timeout);
        final long startTime = (msecs <= 0) ? 0 : System.currentTimeMillis();
        long waitTime = msecs;
        if (this.completed) {
            return getResult();
        } else if (waitTime <= 0) {
            throw new TimeoutException();
        } else {
            for (;;) {
                wait(waitTime);
                if (this.completed) {
                    return getResult();
                } else {
                    waitTime = msecs - (System.currentTimeMillis() - startTime);
                    if (waitTime <= 0) {
                        throw new TimeoutException();
                    }
                }
            }
        }
    }

    public boolean completed(final T result) {
        synchronized(this) {
            if (this.completed) {
                return false;
            }
            this.completed = true;
            this.result = result;
            notifyAll();
        }
        if (this.callback != null) {
            this.callback.completed(result);
        }
        return true;
    }

    public boolean failed(final Exception exception) {
        synchronized(this) {
            if (this.completed) {
                return false;
            }
            this.completed = true;
            this.ex = exception;
            notifyAll();
        }
        if (this.callback != null) {
            this.callback.failed(exception);
        }
        return true;
    }

    public boolean cancel(final boolean mayInterruptIfRunning) {
        synchronized(this) {
            if (this.completed) {
                return false;
            }
            this.completed = true;
            this.cancelled = true;
            notifyAll();
        }
        if (this.callback != null) {
            this.callback.cancelled();
        }
        return true;
    }

    public boolean cancel() {
        return cancel(true);
    }

}
