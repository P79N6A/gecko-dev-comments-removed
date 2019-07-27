

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;























@ThreadSafe
public class ExponentialBackOffSchedulingStrategy implements SchedulingStrategy {

    public static final long DEFAULT_BACK_OFF_RATE = 10;
    public static final long DEFAULT_INITIAL_EXPIRY_IN_MILLIS = TimeUnit.SECONDS.toMillis(6);
    public static final long DEFAULT_MAX_EXPIRY_IN_MILLIS = TimeUnit.SECONDS.toMillis(86400);

    private final long backOffRate;
    private final long initialExpiryInMillis;
    private final long maxExpiryInMillis;

    private final ScheduledExecutorService executor;

    







    public ExponentialBackOffSchedulingStrategy(final CacheConfig cacheConfig) {
        this(cacheConfig,
                DEFAULT_BACK_OFF_RATE,
                DEFAULT_INITIAL_EXPIRY_IN_MILLIS,
                DEFAULT_MAX_EXPIRY_IN_MILLIS);
    }

    










    public ExponentialBackOffSchedulingStrategy(
            final CacheConfig cacheConfig,
            final long backOffRate,
            final long initialExpiryInMillis,
            final long maxExpiryInMillis) {
        this(createThreadPoolFromCacheConfig(cacheConfig),
                backOffRate,
                initialExpiryInMillis,
                maxExpiryInMillis);
    }

    private static ScheduledThreadPoolExecutor createThreadPoolFromCacheConfig(
            final CacheConfig cacheConfig) {
        final ScheduledThreadPoolExecutor scheduledThreadPoolExecutor = new ScheduledThreadPoolExecutor(
                cacheConfig.getAsynchronousWorkersMax());
        scheduledThreadPoolExecutor.setExecuteExistingDelayedTasksAfterShutdownPolicy(false);
        return scheduledThreadPoolExecutor;
    }

    ExponentialBackOffSchedulingStrategy(
            final ScheduledExecutorService executor,
            final long backOffRate,
            final long initialExpiryInMillis,
            final long maxExpiryInMillis) {
        this.executor = checkNotNull("executor", executor);
        this.backOffRate = checkNotNegative("backOffRate", backOffRate);
        this.initialExpiryInMillis = checkNotNegative("initialExpiryInMillis", initialExpiryInMillis);
        this.maxExpiryInMillis = checkNotNegative("maxExpiryInMillis", maxExpiryInMillis);
    }

    public void schedule(
            final AsynchronousValidationRequest revalidationRequest) {
        checkNotNull("revalidationRequest", revalidationRequest);
        final int consecutiveFailedAttempts = revalidationRequest.getConsecutiveFailedAttempts();
        final long delayInMillis = calculateDelayInMillis(consecutiveFailedAttempts);
        executor.schedule(revalidationRequest, delayInMillis, TimeUnit.MILLISECONDS);
    }

    public void close() {
        executor.shutdown();
    }

    public long getBackOffRate() {
        return backOffRate;
    }

    public long getInitialExpiryInMillis() {
        return initialExpiryInMillis;
    }

    public long getMaxExpiryInMillis() {
        return maxExpiryInMillis;
    }

    protected long calculateDelayInMillis(final int consecutiveFailedAttempts) {
        if (consecutiveFailedAttempts > 0) {
            final long delayInSeconds = (long) (initialExpiryInMillis *
                    Math.pow(backOffRate, consecutiveFailedAttempts - 1));
            return Math.min(delayInSeconds, maxExpiryInMillis);
        }
        else {
            return 0;
        }
    }

    protected static <T> T checkNotNull(final String parameterName, final T value) {
        if (value == null) {
            throw new IllegalArgumentException(parameterName + " may not be null");
        }
        return value;
    }

    protected static long checkNotNegative(final String parameterName, final long value) {
        if (value < 0) {
            throw new IllegalArgumentException(parameterName + " may not be negative");
        }
        return value;
    }
}
