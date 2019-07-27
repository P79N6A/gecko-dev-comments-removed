

























package ch.boye.httpclientandroidlib.client.cache;

import java.io.Serializable;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.utils.DateUtils;
import ch.boye.httpclientandroidlib.message.HeaderGroup;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;










@Immutable
public class HttpCacheEntry implements Serializable {

    private static final long serialVersionUID = -6300496422359477413L;

    private final Date requestDate;
    private final Date responseDate;
    private final StatusLine statusLine;
    private final HeaderGroup responseHeaders;
    private final Resource resource;
    private final Map<String,String> variantMap;
    private final Date date;

    

















    public HttpCacheEntry(
            final Date requestDate,
            final Date responseDate,
            final StatusLine statusLine,
            final Header[] responseHeaders,
            final Resource resource,
            final Map<String,String> variantMap) {
        super();
        Args.notNull(requestDate, "Request date");
        Args.notNull(responseDate, "Response date");
        Args.notNull(statusLine, "Status line");
        Args.notNull(responseHeaders, "Response headers");
        this.requestDate = requestDate;
        this.responseDate = responseDate;
        this.statusLine = statusLine;
        this.responseHeaders = new HeaderGroup();
        this.responseHeaders.setHeaders(responseHeaders);
        this.resource = resource;
        this.variantMap = variantMap != null
            ? new HashMap<String,String>(variantMap)
            : null;
        this.date = parseDate();
    }

    














    public HttpCacheEntry(final Date requestDate, final Date responseDate, final StatusLine statusLine,
            final Header[] responseHeaders, final Resource resource) {
        this(requestDate, responseDate, statusLine, responseHeaders, resource,
                new HashMap<String,String>());
    }

    



    private Date parseDate() {
        final Header dateHdr = getFirstHeader(HTTP.DATE_HEADER);
        if (dateHdr == null) {
            return null;
        }
        return DateUtils.parseDate(dateHdr.getValue());
    }

    



    public StatusLine getStatusLine() {
        return this.statusLine;
    }

    



    public ProtocolVersion getProtocolVersion() {
        return this.statusLine.getProtocolVersion();
    }

    



    public String getReasonPhrase() {
        return this.statusLine.getReasonPhrase();
    }

    



    public int getStatusCode() {
        return this.statusLine.getStatusCode();
    }

    




    public Date getRequestDate() {
        return requestDate;
    }

    



    public Date getResponseDate() {
        return responseDate;
    }

    


    public Header[] getAllHeaders() {
        return responseHeaders.getAllHeaders();
    }

    



    public Header getFirstHeader(final String name) {
        return responseHeaders.getFirstHeader(name);
    }

    



    public Header[] getHeaders(final String name) {
        return responseHeaders.getHeaders(name);
    }

    





    public Date getDate() {
        return date;
    }

    


    public Resource getResource() {
        return this.resource;
    }

    





    public boolean hasVariants() {
        return getFirstHeader(HeaderConstants.VARY) != null;
    }

    








    public Map<String, String> getVariantMap() {
        return Collections.unmodifiableMap(variantMap);
    }

    



    @Override
    public String toString() {
        return "[request date=" + this.requestDate + "; response date=" + this.responseDate
                + "; statusLine=" + this.statusLine + "]";
    }

}
