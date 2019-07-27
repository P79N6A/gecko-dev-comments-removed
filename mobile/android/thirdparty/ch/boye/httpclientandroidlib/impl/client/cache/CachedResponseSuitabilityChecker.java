

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.Date;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;







@Immutable
class CachedResponseSuitabilityChecker {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final boolean sharedCache;
    private final boolean useHeuristicCaching;
    private final float heuristicCoefficient;
    private final long heuristicDefaultLifetime;
    private final CacheValidityPolicy validityStrategy;

    CachedResponseSuitabilityChecker(final CacheValidityPolicy validityStrategy,
            final CacheConfig config) {
        super();
        this.validityStrategy = validityStrategy;
        this.sharedCache = config.isSharedCache();
        this.useHeuristicCaching = config.isHeuristicCachingEnabled();
        this.heuristicCoefficient = config.getHeuristicCoefficient();
        this.heuristicDefaultLifetime = config.getHeuristicDefaultLifetime();
    }

    CachedResponseSuitabilityChecker(final CacheConfig config) {
        this(new CacheValidityPolicy(), config);
    }

    private boolean isFreshEnough(final HttpCacheEntry entry, final HttpRequest request, final Date now) {
        if (validityStrategy.isResponseFresh(entry, now)) {
            return true;
        }
        if (useHeuristicCaching &&
                validityStrategy.isResponseHeuristicallyFresh(entry, now, heuristicCoefficient, heuristicDefaultLifetime)) {
            return true;
        }
        if (originInsistsOnFreshness(entry)) {
            return false;
        }
        final long maxstale = getMaxStale(request);
        if (maxstale == -1) {
            return false;
        }
        return (maxstale > validityStrategy.getStalenessSecs(entry, now));
    }

    private boolean originInsistsOnFreshness(final HttpCacheEntry entry) {
        if (validityStrategy.mustRevalidate(entry)) {
            return true;
        }
        if (!sharedCache) {
            return false;
        }
        return validityStrategy.proxyRevalidate(entry) ||
            validityStrategy.hasCacheControlDirective(entry, "s-maxage");
    }

    private long getMaxStale(final HttpRequest request) {
        long maxstale = -1;
        for(final Header h : request.getHeaders(HeaderConstants.CACHE_CONTROL)) {
            for(final HeaderElement elt : h.getElements()) {
                if (HeaderConstants.CACHE_CONTROL_MAX_STALE.equals(elt.getName())) {
                    if ((elt.getValue() == null || "".equals(elt.getValue().trim()))
                            && maxstale == -1) {
                        maxstale = Long.MAX_VALUE;
                    } else {
                        try {
                            long val = Long.parseLong(elt.getValue());
                            if (val < 0) {
                                val = 0;
                            }
                            if (maxstale == -1 || val < maxstale) {
                                maxstale = val;
                            }
                        } catch (final NumberFormatException nfe) {
                            
                            maxstale = 0;
                        }
                    }
                }
            }
        }
        return maxstale;
    }

    













    public boolean canCachedResponseBeUsed(final HttpHost host, final HttpRequest request, final HttpCacheEntry entry, final Date now) {

        if (!isFreshEnough(entry, request, now)) {
            log.trace("Cache entry was not fresh enough");
            return false;
        }

        if (!validityStrategy.contentLengthHeaderMatchesActualLength(entry)) {
            log.debug("Cache entry Content-Length and header information do not match");
            return false;
        }

        if (hasUnsupportedConditionalHeaders(request)) {
            log.debug("Request contained conditional headers we don't handle");
            return false;
        }

        if (!isConditional(request) && entry.getStatusCode() == HttpStatus.SC_NOT_MODIFIED) {
        return false;
        }

        if (isConditional(request) && !allConditionalsMatch(request, entry, now)) {
            return false;
        }

        for (final Header ccHdr : request.getHeaders(HeaderConstants.CACHE_CONTROL)) {
            for (final HeaderElement elt : ccHdr.getElements()) {
                if (HeaderConstants.CACHE_CONTROL_NO_CACHE.equals(elt.getName())) {
                    log.trace("Response contained NO CACHE directive, cache was not suitable");
                    return false;
                }

                if (HeaderConstants.CACHE_CONTROL_NO_STORE.equals(elt.getName())) {
                    log.trace("Response contained NO STORE directive, cache was not suitable");
                    return false;
                }

                if (HeaderConstants.CACHE_CONTROL_MAX_AGE.equals(elt.getName())) {
                    try {
                        final int maxage = Integer.parseInt(elt.getValue());
                        if (validityStrategy.getCurrentAgeSecs(entry, now) > maxage) {
                            log.trace("Response from cache was NOT suitable due to max age");
                            return false;
                        }
                    } catch (final NumberFormatException ex) {
                        
                        log.debug("Response from cache was malformed" + ex.getMessage());
                        return false;
                    }
                }

                if (HeaderConstants.CACHE_CONTROL_MAX_STALE.equals(elt.getName())) {
                    try {
                        final int maxstale = Integer.parseInt(elt.getValue());
                        if (validityStrategy.getFreshnessLifetimeSecs(entry) > maxstale) {
                            log.trace("Response from cache was not suitable due to Max stale freshness");
                            return false;
                        }
                    } catch (final NumberFormatException ex) {
                        
                        log.debug("Response from cache was malformed: " + ex.getMessage());
                        return false;
                    }
                }

                if (HeaderConstants.CACHE_CONTROL_MIN_FRESH.equals(elt.getName())) {
                    try {
                        final long minfresh = Long.parseLong(elt.getValue());
                        if (minfresh < 0L) {
                            return false;
                        }
                        final long age = validityStrategy.getCurrentAgeSecs(entry, now);
                        final long freshness = validityStrategy.getFreshnessLifetimeSecs(entry);
                        if (freshness - age < minfresh) {
                            log.trace("Response from cache was not suitable due to min fresh " +
                                    "freshness requirement");
                            return false;
                        }
                    } catch (final NumberFormatException ex) {
                        
                        log.debug("Response from cache was malformed: " + ex.getMessage());
                        return false;
                    }
                }
            }
        }

        log.trace("Response from cache was suitable");
        return true;
    }

    




    public boolean isConditional(final HttpRequest request) {
        return hasSupportedEtagValidator(request) || hasSupportedLastModifiedValidator(request);
    }

    






    public boolean allConditionalsMatch(final HttpRequest request, final HttpCacheEntry entry, final Date now) {
        final boolean hasEtagValidator = hasSupportedEtagValidator(request);
        final boolean hasLastModifiedValidator = hasSupportedLastModifiedValidator(request);

        final boolean etagValidatorMatches = (hasEtagValidator) && etagValidatorMatches(request, entry);
        final boolean lastModifiedValidatorMatches = (hasLastModifiedValidator) && lastModifiedValidatorMatches(request, entry, now);

        if ((hasEtagValidator && hasLastModifiedValidator)
            && !(etagValidatorMatches && lastModifiedValidatorMatches)) {
            return false;
        } else if (hasEtagValidator && !etagValidatorMatches) {
            return false;
        }

        if (hasLastModifiedValidator && !lastModifiedValidatorMatches) {
            return false;
        }
        return true;
    }

    private boolean hasUnsupportedConditionalHeaders(final HttpRequest request) {
        return (request.getFirstHeader(HeaderConstants.IF_RANGE) != null
                || request.getFirstHeader(HeaderConstants.IF_MATCH) != null
                || hasValidDateField(request, HeaderConstants.IF_UNMODIFIED_SINCE));
    }

    private boolean hasSupportedEtagValidator(final HttpRequest request) {
        return request.containsHeader(HeaderConstants.IF_NONE_MATCH);
    }

    private boolean hasSupportedLastModifiedValidator(final HttpRequest request) {
        return hasValidDateField(request, HeaderConstants.IF_MODIFIED_SINCE);
    }

    





    private boolean etagValidatorMatches(final HttpRequest request, final HttpCacheEntry entry) {
        final Header etagHeader = entry.getFirstHeader(HeaderConstants.ETAG);
        final String etag = (etagHeader != null) ? etagHeader.getValue() : null;
        final Header[] ifNoneMatch = request.getHeaders(HeaderConstants.IF_NONE_MATCH);
        if (ifNoneMatch != null) {
            for (final Header h : ifNoneMatch) {
                for (final HeaderElement elt : h.getElements()) {
                    final String reqEtag = elt.toString();
                    if (("*".equals(reqEtag) && etag != null)
                            || reqEtag.equals(etag)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    







    private boolean lastModifiedValidatorMatches(final HttpRequest request, final HttpCacheEntry entry, final Date now) {
        final Header lastModifiedHeader = entry.getFirstHeader(HeaderConstants.LAST_MODIFIED);
        Date lastModified = null;
        if (lastModifiedHeader != null) {
            lastModified = DateUtils.parseDate(lastModifiedHeader.getValue());
        }
        if (lastModified == null) {
            return false;
        }

        for (final Header h : request.getHeaders(HeaderConstants.IF_MODIFIED_SINCE)) {
            final Date ifModifiedSince = DateUtils.parseDate(h.getValue());
            if (ifModifiedSince != null) {
                if (ifModifiedSince.after(now) || lastModified.after(ifModifiedSince)) {
                    return false;
                }
            }
        }
        return true;
    }

    private boolean hasValidDateField(final HttpRequest request, final String headerName) {
        for(final Header h : request.getHeaders(headerName)) {
            final Date date = DateUtils.parseDate(h.getValue());
            return date != null;
        }
        return false;
    }
}
