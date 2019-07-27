

























package ch.boye.httpclientandroidlib.impl.client.cache;

import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;


class Variant {

    private final String variantKey;
    private final String cacheKey;
    private final HttpCacheEntry entry;

    public Variant(final String variantKey, final String cacheKey, final HttpCacheEntry entry) {
        this.variantKey = variantKey;
        this.cacheKey = cacheKey;
        this.entry = entry;
    }

    public String getVariantKey() {
        return variantKey;
    }

    public String getCacheKey() {
        return cacheKey;
    }

    public HttpCacheEntry getEntry() {
        return entry;
    }
}
