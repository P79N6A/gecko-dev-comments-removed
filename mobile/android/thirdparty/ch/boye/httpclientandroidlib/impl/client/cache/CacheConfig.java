

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.util.Args;









































































public class CacheConfig implements Cloneable {

    


    public final static int DEFAULT_MAX_OBJECT_SIZE_BYTES = 8192;

    


    public final static int DEFAULT_MAX_CACHE_ENTRIES = 1000;

    


    public final static int DEFAULT_MAX_UPDATE_RETRIES = 1;

    

    public final static boolean DEFAULT_303_CACHING_ENABLED = false;

    

    public final static boolean DEFAULT_WEAK_ETAG_ON_PUTDELETE_ALLOWED = false;

    

    public final static boolean DEFAULT_HEURISTIC_CACHING_ENABLED = false;

    


    public final static float DEFAULT_HEURISTIC_COEFFICIENT = 0.1f;

    


    public final static long DEFAULT_HEURISTIC_LIFETIME = 0;

    


    public static final int DEFAULT_ASYNCHRONOUS_WORKERS_MAX = 1;

    


    public static final int DEFAULT_ASYNCHRONOUS_WORKERS_CORE = 1;

    


    public static final int DEFAULT_ASYNCHRONOUS_WORKER_IDLE_LIFETIME_SECS = 60;

    

    public static final int DEFAULT_REVALIDATION_QUEUE_SIZE = 100;

    public static final CacheConfig DEFAULT = new Builder().build();

    
    private long maxObjectSize;
    private int maxCacheEntries;
    private int maxUpdateRetries;
    private boolean allow303Caching;
    private boolean weakETagOnPutDeleteAllowed;
    private boolean heuristicCachingEnabled;
    private float heuristicCoefficient;
    private long heuristicDefaultLifetime;
    private boolean isSharedCache;
    private int asynchronousWorkersMax;
    private int asynchronousWorkersCore;
    private int asynchronousWorkerIdleLifetimeSecs;
    private int revalidationQueueSize;
    private boolean neverCacheHTTP10ResponsesWithQuery;

    


    @Deprecated
    public CacheConfig() {
        super();
        this.maxObjectSize = DEFAULT_MAX_OBJECT_SIZE_BYTES;
        this.maxCacheEntries = DEFAULT_MAX_CACHE_ENTRIES;
        this.maxUpdateRetries = DEFAULT_MAX_UPDATE_RETRIES;
        this.allow303Caching = DEFAULT_303_CACHING_ENABLED;
        this.weakETagOnPutDeleteAllowed = DEFAULT_WEAK_ETAG_ON_PUTDELETE_ALLOWED;
        this.heuristicCachingEnabled = DEFAULT_HEURISTIC_CACHING_ENABLED;
        this.heuristicCoefficient = DEFAULT_HEURISTIC_COEFFICIENT;
        this.heuristicDefaultLifetime = DEFAULT_HEURISTIC_LIFETIME;
        this.isSharedCache = true;
        this.asynchronousWorkersMax = DEFAULT_ASYNCHRONOUS_WORKERS_MAX;
        this.asynchronousWorkersCore = DEFAULT_ASYNCHRONOUS_WORKERS_CORE;
        this.asynchronousWorkerIdleLifetimeSecs = DEFAULT_ASYNCHRONOUS_WORKER_IDLE_LIFETIME_SECS;
        this.revalidationQueueSize = DEFAULT_REVALIDATION_QUEUE_SIZE;
    }

    CacheConfig(
            final long maxObjectSize,
            final int maxCacheEntries,
            final int maxUpdateRetries,
            final boolean allow303Caching,
            final boolean weakETagOnPutDeleteAllowed,
            final boolean heuristicCachingEnabled,
            final float heuristicCoefficient,
            final long heuristicDefaultLifetime,
            final boolean isSharedCache,
            final int asynchronousWorkersMax,
            final int asynchronousWorkersCore,
            final int asynchronousWorkerIdleLifetimeSecs,
            final int revalidationQueueSize,
            final boolean neverCacheHTTP10ResponsesWithQuery) {
        super();
        this.maxObjectSize = maxObjectSize;
        this.maxCacheEntries = maxCacheEntries;
        this.maxUpdateRetries = maxUpdateRetries;
        this.allow303Caching = allow303Caching;
        this.weakETagOnPutDeleteAllowed = weakETagOnPutDeleteAllowed;
        this.heuristicCachingEnabled = heuristicCachingEnabled;
        this.heuristicCoefficient = heuristicCoefficient;
        this.heuristicDefaultLifetime = heuristicDefaultLifetime;
        this.isSharedCache = isSharedCache;
        this.asynchronousWorkersMax = asynchronousWorkersMax;
        this.asynchronousWorkersCore = asynchronousWorkersCore;
        this.asynchronousWorkerIdleLifetimeSecs = asynchronousWorkerIdleLifetimeSecs;
        this.revalidationQueueSize = revalidationQueueSize;
    }

    





    @Deprecated
    public int getMaxObjectSizeBytes() {
        return maxObjectSize > Integer.MAX_VALUE ? Integer.MAX_VALUE : (int) maxObjectSize;
    }

    





    @Deprecated
    public void setMaxObjectSizeBytes(final int maxObjectSizeBytes) {
        if (maxObjectSizeBytes > Integer.MAX_VALUE) {
            this.maxObjectSize = Integer.MAX_VALUE;
        } else {
            this.maxObjectSize = maxObjectSizeBytes;
        }
    }

    





    public long getMaxObjectSize() {
        return maxObjectSize;
    }

    







    @Deprecated
    public void setMaxObjectSize(final long maxObjectSize) {
        this.maxObjectSize = maxObjectSize;
    }

    




    public boolean isNeverCacheHTTP10ResponsesWithQuery() {
        return neverCacheHTTP10ResponsesWithQuery;
    }

    


    public int getMaxCacheEntries() {
        return maxCacheEntries;
    }

    




    @Deprecated
    public void setMaxCacheEntries(final int maxCacheEntries) {
        this.maxCacheEntries = maxCacheEntries;
    }

    


    public int getMaxUpdateRetries(){
        return maxUpdateRetries;
    }

    




    @Deprecated
    public void setMaxUpdateRetries(final int maxUpdateRetries){
        this.maxUpdateRetries = maxUpdateRetries;
    }

    



    public boolean is303CachingEnabled() {
        return allow303Caching;
    }

    



    public boolean isWeakETagOnPutDeleteAllowed() {
        return weakETagOnPutDeleteAllowed;
    }

    



    public boolean isHeuristicCachingEnabled() {
        return heuristicCachingEnabled;
    }

    






    @Deprecated
    public void setHeuristicCachingEnabled(final boolean heuristicCachingEnabled) {
        this.heuristicCachingEnabled = heuristicCachingEnabled;
    }

    


    public float getHeuristicCoefficient() {
        return heuristicCoefficient;
    }

    









    @Deprecated
    public void setHeuristicCoefficient(final float heuristicCoefficient) {
        this.heuristicCoefficient = heuristicCoefficient;
    }

    



    public long getHeuristicDefaultLifetime() {
        return heuristicDefaultLifetime;
    }

    












    @Deprecated
    public void setHeuristicDefaultLifetime(final long heuristicDefaultLifetimeSecs) {
        this.heuristicDefaultLifetime = heuristicDefaultLifetimeSecs;
    }

    




    public boolean isSharedCache() {
        return isSharedCache;
    }

    







    @Deprecated
    public void setSharedCache(final boolean isSharedCache) {
        this.isSharedCache = isSharedCache;
    }

    




    public int getAsynchronousWorkersMax() {
        return asynchronousWorkersMax;
    }

    







    @Deprecated
    public void setAsynchronousWorkersMax(final int max) {
        this.asynchronousWorkersMax = max;
    }

    



    public int getAsynchronousWorkersCore() {
        return asynchronousWorkersCore;
    }

    







    @Deprecated
    public void setAsynchronousWorkersCore(final int min) {
        this.asynchronousWorkersCore = min;
    }

    





    public int getAsynchronousWorkerIdleLifetimeSecs() {
        return asynchronousWorkerIdleLifetimeSecs;
    }

    








    @Deprecated
    public void setAsynchronousWorkerIdleLifetimeSecs(final int secs) {
        this.asynchronousWorkerIdleLifetimeSecs = secs;
    }

    


    public int getRevalidationQueueSize() {
        return revalidationQueueSize;
    }

    




    @Deprecated
    public void setRevalidationQueueSize(final int size) {
        this.revalidationQueueSize = size;
    }

    @Override
    protected CacheConfig clone() throws CloneNotSupportedException {
        return (CacheConfig) super.clone();
    }

    public static Builder custom() {
        return new Builder();
    }

    public static Builder copy(final CacheConfig config) {
        Args.notNull(config, "Cache config");
        return new Builder()
            .setMaxObjectSize(config.getMaxObjectSize())
            .setMaxCacheEntries(config.getMaxCacheEntries())
            .setMaxUpdateRetries(config.getMaxUpdateRetries())
            .setHeuristicCachingEnabled(config.isHeuristicCachingEnabled())
            .setHeuristicCoefficient(config.getHeuristicCoefficient())
            .setHeuristicDefaultLifetime(config.getHeuristicDefaultLifetime())
            .setSharedCache(config.isSharedCache())
            .setAsynchronousWorkersMax(config.getAsynchronousWorkersMax())
            .setAsynchronousWorkersCore(config.getAsynchronousWorkersCore())
            .setAsynchronousWorkerIdleLifetimeSecs(config.getAsynchronousWorkerIdleLifetimeSecs())
            .setRevalidationQueueSize(config.getRevalidationQueueSize())
            .setNeverCacheHTTP10ResponsesWithQueryString(config.isNeverCacheHTTP10ResponsesWithQuery());
    }


    public static class Builder {

        private long maxObjectSize;
        private int maxCacheEntries;
        private int maxUpdateRetries;
        private boolean allow303Caching;
        private boolean weakETagOnPutDeleteAllowed;
        private boolean heuristicCachingEnabled;
        private float heuristicCoefficient;
        private long heuristicDefaultLifetime;
        private boolean isSharedCache;
        private int asynchronousWorkersMax;
        private int asynchronousWorkersCore;
        private int asynchronousWorkerIdleLifetimeSecs;
        private int revalidationQueueSize;
        private boolean neverCacheHTTP10ResponsesWithQuery;

        Builder() {
            this.maxObjectSize = DEFAULT_MAX_OBJECT_SIZE_BYTES;
            this.maxCacheEntries = DEFAULT_MAX_CACHE_ENTRIES;
            this.maxUpdateRetries = DEFAULT_MAX_UPDATE_RETRIES;
            this.allow303Caching = DEFAULT_303_CACHING_ENABLED;
            this.weakETagOnPutDeleteAllowed = DEFAULT_WEAK_ETAG_ON_PUTDELETE_ALLOWED;
            this.heuristicCachingEnabled = false;
            this.heuristicCoefficient = DEFAULT_HEURISTIC_COEFFICIENT;
            this.heuristicDefaultLifetime = DEFAULT_HEURISTIC_LIFETIME;
            this.isSharedCache = true;
            this.asynchronousWorkersMax = DEFAULT_ASYNCHRONOUS_WORKERS_MAX;
            this.asynchronousWorkersCore = DEFAULT_ASYNCHRONOUS_WORKERS_CORE;
            this.asynchronousWorkerIdleLifetimeSecs = DEFAULT_ASYNCHRONOUS_WORKER_IDLE_LIFETIME_SECS;
            this.revalidationQueueSize = DEFAULT_REVALIDATION_QUEUE_SIZE;
        }

        



        public Builder setMaxObjectSize(final long maxObjectSize) {
            this.maxObjectSize = maxObjectSize;
            return this;
        }

        


        public Builder setMaxCacheEntries(final int maxCacheEntries) {
            this.maxCacheEntries = maxCacheEntries;
            return this;
        }

        


        public Builder setMaxUpdateRetries(final int maxUpdateRetries) {
            this.maxUpdateRetries = maxUpdateRetries;
            return this;
        }

        




        public Builder setAllow303Caching(final boolean allow303Caching) {
            this.allow303Caching = allow303Caching;
            return this;
        }

        




        public Builder setWeakETagOnPutDeleteAllowed(final boolean weakETagOnPutDeleteAllowed) {
            this.weakETagOnPutDeleteAllowed = weakETagOnPutDeleteAllowed;
            return this;
        }

        




        public Builder setHeuristicCachingEnabled(final boolean heuristicCachingEnabled) {
            this.heuristicCachingEnabled = heuristicCachingEnabled;
            return this;
        }

        







        public Builder setHeuristicCoefficient(final float heuristicCoefficient) {
            this.heuristicCoefficient = heuristicCoefficient;
            return this;
        }

        










        public Builder setHeuristicDefaultLifetime(final long heuristicDefaultLifetime) {
            this.heuristicDefaultLifetime = heuristicDefaultLifetime;
            return this;
        }

        





        public Builder setSharedCache(final boolean isSharedCache) {
            this.isSharedCache = isSharedCache;
            return this;
        }

        





        public Builder setAsynchronousWorkersMax(final int asynchronousWorkersMax) {
            this.asynchronousWorkersMax = asynchronousWorkersMax;
            return this;
        }

        





        public Builder setAsynchronousWorkersCore(final int asynchronousWorkersCore) {
            this.asynchronousWorkersCore = asynchronousWorkersCore;
            return this;
        }

        






        public Builder setAsynchronousWorkerIdleLifetimeSecs(final int asynchronousWorkerIdleLifetimeSecs) {
            this.asynchronousWorkerIdleLifetimeSecs = asynchronousWorkerIdleLifetimeSecs;
            return this;
        }

        


        public Builder setRevalidationQueueSize(final int revalidationQueueSize) {
            this.revalidationQueueSize = revalidationQueueSize;
            return this;
        }

        






        public Builder setNeverCacheHTTP10ResponsesWithQueryString(
                final boolean neverCacheHTTP10ResponsesWithQuery) {
            this.neverCacheHTTP10ResponsesWithQuery = neverCacheHTTP10ResponsesWithQuery;
            return this;
        }

        public CacheConfig build() {
            return new CacheConfig(
                    maxObjectSize,
                    maxCacheEntries,
                    maxUpdateRetries,
                    allow303Caching,
                    weakETagOnPutDeleteAllowed,
                    heuristicCachingEnabled,
                    heuristicCoefficient,
                    heuristicDefaultLifetime,
                    isSharedCache,
                    asynchronousWorkersMax,
                    asynchronousWorkersCore,
                    asynchronousWorkerIdleLifetimeSecs,
                    revalidationQueueSize,
                    neverCacheHTTP10ResponsesWithQuery);
        }

    }

    @Override
    public String toString() {
        final StringBuilder builder = new StringBuilder();
        builder.append("[maxObjectSize=").append(this.maxObjectSize)
                .append(", maxCacheEntries=").append(this.maxCacheEntries)
                .append(", maxUpdateRetries=").append(this.maxUpdateRetries)
                .append(", 303CachingEnabled=").append(this.allow303Caching)
                .append(", weakETagOnPutDeleteAllowed=").append(this.weakETagOnPutDeleteAllowed)
                .append(", heuristicCachingEnabled=").append(this.heuristicCachingEnabled)
                .append(", heuristicCoefficient=").append(this.heuristicCoefficient)
                .append(", heuristicDefaultLifetime=").append(this.heuristicDefaultLifetime)
                .append(", isSharedCache=").append(this.isSharedCache)
                .append(", asynchronousWorkersMax=").append(this.asynchronousWorkersMax)
                .append(", asynchronousWorkersCore=").append(this.asynchronousWorkersCore)
                .append(", asynchronousWorkerIdleLifetimeSecs=").append(this.asynchronousWorkerIdleLifetimeSecs)
                .append(", revalidationQueueSize=").append(this.revalidationQueueSize)
                .append(", neverCacheHTTP10ResponsesWithQuery=").append(this.neverCacheHTTP10ResponsesWithQuery)
                .append("]");
        return builder.toString();
    }

}
