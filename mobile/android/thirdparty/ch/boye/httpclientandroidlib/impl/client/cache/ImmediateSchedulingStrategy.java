

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;







@ThreadSafe
public class ImmediateSchedulingStrategy implements SchedulingStrategy {

    private final ExecutorService executor;

    








    public ImmediateSchedulingStrategy(final CacheConfig cacheConfig) {
        this(new ThreadPoolExecutor(
                cacheConfig.getAsynchronousWorkersCore(),
                cacheConfig.getAsynchronousWorkersMax(),
                cacheConfig.getAsynchronousWorkerIdleLifetimeSecs(),
                TimeUnit.SECONDS,
                new ArrayBlockingQueue<Runnable>(cacheConfig.getRevalidationQueueSize()))
        );
    }

    ImmediateSchedulingStrategy(final ExecutorService executor) {
        this.executor = executor;
    }

    public void schedule(final AsynchronousValidationRequest revalidationRequest) {
        if (revalidationRequest == null) {
            throw new IllegalArgumentException("AsynchronousValidationRequest may not be null");
        }

        executor.execute(revalidationRequest);
    }

    public void close() {
        executor.shutdown();
    }

    


    void awaitTermination(final long timeout, final TimeUnit unit) throws InterruptedException {
        executor.awaitTermination(timeout, unit);
    }
}
