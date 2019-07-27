

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.File;

import ch.boye.httpclientandroidlib.client.cache.HttpCacheInvalidator;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheStorage;
import ch.boye.httpclientandroidlib.client.cache.ResourceFactory;
import ch.boye.httpclientandroidlib.impl.client.HttpClientBuilder;
import ch.boye.httpclientandroidlib.impl.execchain.ClientExecChain;







public class CachingHttpClientBuilder extends HttpClientBuilder {

    private ResourceFactory resourceFactory;
    private HttpCacheStorage storage;
    private File cacheDir;
    private CacheConfig cacheConfig;
    private SchedulingStrategy schedulingStrategy;
    private HttpCacheInvalidator httpCacheInvalidator;

    public static CachingHttpClientBuilder create() {
        return new CachingHttpClientBuilder();
    }

    protected CachingHttpClientBuilder() {
        super();
    }

    public final CachingHttpClientBuilder setResourceFactory(
            final ResourceFactory resourceFactory) {
        this.resourceFactory = resourceFactory;
        return this;
    }

    public final CachingHttpClientBuilder setHttpCacheStorage(
            final HttpCacheStorage storage) {
        this.storage = storage;
        return this;
    }

    public final CachingHttpClientBuilder setCacheDir(
            final File cacheDir) {
        this.cacheDir = cacheDir;
        return this;
    }

    public final CachingHttpClientBuilder setCacheConfig(
            final CacheConfig cacheConfig) {
        this.cacheConfig = cacheConfig;
        return this;
    }

    public final CachingHttpClientBuilder setSchedulingStrategy(
            final SchedulingStrategy schedulingStrategy) {
        this.schedulingStrategy = schedulingStrategy;
        return this;
    }

    public final CachingHttpClientBuilder setHttpCacheInvalidator(
            final HttpCacheInvalidator cacheInvalidator) {
        this.httpCacheInvalidator = cacheInvalidator;
        return this;
    }

    @Override
    protected ClientExecChain decorateMainExec(final ClientExecChain mainExec) {
        final CacheConfig config = this.cacheConfig != null ? this.cacheConfig : CacheConfig.DEFAULT;
        ResourceFactory resourceFactory = this.resourceFactory;
        if (resourceFactory == null) {
            if (this.cacheDir == null) {
                resourceFactory = new HeapResourceFactory();
            } else {
                resourceFactory = new FileResourceFactory(cacheDir);
            }
        }
        HttpCacheStorage storage = this.storage;
        if (storage == null) {
            if (this.cacheDir == null) {
                storage = new BasicHttpCacheStorage(config);
            } else {
                final ManagedHttpCacheStorage managedStorage = new ManagedHttpCacheStorage(config);
                addCloseable(managedStorage);
                storage = managedStorage;
            }
        }
        final AsynchronousValidator revalidator = createAsynchronousRevalidator(config);
        final CacheKeyGenerator uriExtractor = new CacheKeyGenerator();

        HttpCacheInvalidator cacheInvalidator = this.httpCacheInvalidator;
        if (cacheInvalidator == null) {
            cacheInvalidator = new CacheInvalidator(uriExtractor, storage);
        }

        return new CachingExec(mainExec,
                new BasicHttpCache(
                        resourceFactory,
                        storage, config,
                        uriExtractor,
                        cacheInvalidator), config, revalidator);
    }

    private AsynchronousValidator createAsynchronousRevalidator(final CacheConfig config) {
        if (config.getAsynchronousWorkersMax() > 0) {
            final SchedulingStrategy configuredSchedulingStrategy = createSchedulingStrategy(config);
            final AsynchronousValidator revalidator = new AsynchronousValidator(
                    configuredSchedulingStrategy);
            addCloseable(revalidator);
            return revalidator;
        }
        return null;
    }

    @SuppressWarnings("resource")
    private SchedulingStrategy createSchedulingStrategy(final CacheConfig config) {
        return schedulingStrategy != null ? schedulingStrategy : new ImmediateSchedulingStrategy(config);
    }

}
