

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.ListIterator;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.cache.Resource;
import ch.boye.httpclientandroidlib.client.cache.ResourceFactory;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;








@Immutable
class CacheEntryUpdater {

    private final ResourceFactory resourceFactory;

    CacheEntryUpdater() {
        this(new HeapResourceFactory());
    }

    CacheEntryUpdater(final ResourceFactory resourceFactory) {
        super();
        this.resourceFactory = resourceFactory;
    }

    











    public HttpCacheEntry updateCacheEntry(
            final String requestId,
            final HttpCacheEntry entry,
            final Date requestDate,
            final Date responseDate,
            final HttpResponse response) throws IOException {
        Args.check(response.getStatusLine().getStatusCode() == HttpStatus.SC_NOT_MODIFIED,
                "Response must have 304 status code");
        final Header[] mergedHeaders = mergeHeaders(entry, response);
        Resource resource = null;
        if (entry.getResource() != null) {
            resource = resourceFactory.copy(requestId, entry.getResource());
        }
        return new HttpCacheEntry(
                requestDate,
                responseDate,
                entry.getStatusLine(),
                mergedHeaders,
                resource);
    }

    protected Header[] mergeHeaders(final HttpCacheEntry entry, final HttpResponse response) {

        if (entryAndResponseHaveDateHeader(entry, response)
                && entryDateHeaderNewerThenResponse(entry, response)) {
            
            return entry.getAllHeaders();
        }

        final List<Header> cacheEntryHeaderList = new ArrayList<Header>(Arrays.asList(entry
                .getAllHeaders()));
        removeCacheHeadersThatMatchResponse(cacheEntryHeaderList, response);
        removeCacheEntry1xxWarnings(cacheEntryHeaderList, entry);
        cacheEntryHeaderList.addAll(Arrays.asList(response.getAllHeaders()));

        return cacheEntryHeaderList.toArray(new Header[cacheEntryHeaderList.size()]);
    }

    private void removeCacheHeadersThatMatchResponse(final List<Header> cacheEntryHeaderList,
            final HttpResponse response) {
        for (final Header responseHeader : response.getAllHeaders()) {
            final ListIterator<Header> cacheEntryHeaderListIter = cacheEntryHeaderList.listIterator();

            while (cacheEntryHeaderListIter.hasNext()) {
                final String cacheEntryHeaderName = cacheEntryHeaderListIter.next().getName();

                if (cacheEntryHeaderName.equals(responseHeader.getName())) {
                    cacheEntryHeaderListIter.remove();
                }
            }
        }
    }

    private void removeCacheEntry1xxWarnings(final List<Header> cacheEntryHeaderList, final HttpCacheEntry entry) {
        final ListIterator<Header> cacheEntryHeaderListIter = cacheEntryHeaderList.listIterator();

        while (cacheEntryHeaderListIter.hasNext()) {
            final String cacheEntryHeaderName = cacheEntryHeaderListIter.next().getName();

            if (HeaderConstants.WARNING.equals(cacheEntryHeaderName)) {
                for (final Header cacheEntryWarning : entry.getHeaders(HeaderConstants.WARNING)) {
                    if (cacheEntryWarning.getValue().startsWith("1")) {
                        cacheEntryHeaderListIter.remove();
                    }
                }
            }
        }
    }

    private boolean entryDateHeaderNewerThenResponse(final HttpCacheEntry entry, final HttpResponse response) {
        final Date entryDate = DateUtils.parseDate(entry.getFirstHeader(HTTP.DATE_HEADER)
                .getValue());
        final Date responseDate = DateUtils.parseDate(response.getFirstHeader(HTTP.DATE_HEADER)
                .getValue());
        if (entryDate == null || responseDate == null) {
            return false;
        }
        if (!entryDate.after(responseDate)) {
            return false;
        }
        return true;
    }

    private boolean entryAndResponseHaveDateHeader(final HttpCacheEntry entry, final HttpResponse response) {
        if (entry.getFirstHeader(HTTP.DATE_HEADER) != null
                && response.getFirstHeader(HTTP.DATE_HEADER) != null) {
            return true;
        }

        return false;
    }

}
