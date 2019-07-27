

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.util.Locale;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.message.AbstractHttpMessage;
import ch.boye.httpclientandroidlib.message.BasicStatusLine;
import ch.boye.httpclientandroidlib.params.BasicHttpParams;
import ch.boye.httpclientandroidlib.params.HttpParams;




@SuppressWarnings("deprecation")
@Immutable
final class OptionsHttp11Response extends AbstractHttpMessage implements HttpResponse {

    private final StatusLine statusLine = new BasicStatusLine(HttpVersion.HTTP_1_1,
            HttpStatus.SC_NOT_IMPLEMENTED, "");
    private final ProtocolVersion version = HttpVersion.HTTP_1_1;

    public StatusLine getStatusLine() {
        return statusLine;
    }

    public void setStatusLine(final StatusLine statusline) {
        
    }

    public void setStatusLine(final ProtocolVersion ver, final int code) {
        
    }

    public void setStatusLine(final ProtocolVersion ver, final int code, final String reason) {
        
    }

    public void setStatusCode(final int code) throws IllegalStateException {
        
    }

    public void setReasonPhrase(final String reason) throws IllegalStateException {
        
    }

    public HttpEntity getEntity() {
        return null;
    }

    public void setEntity(final HttpEntity entity) {
        
    }

    public Locale getLocale() {
        return null;
    }

    public void setLocale(final Locale loc) {
        
    }

    public ProtocolVersion getProtocolVersion() {
        return version;
    }

    @Override
    public boolean containsHeader(final String name) {
        return this.headergroup.containsHeader(name);
    }

    @Override
    public Header[] getHeaders(final String name) {
        return this.headergroup.getHeaders(name);
    }

    @Override
    public Header getFirstHeader(final String name) {
        return this.headergroup.getFirstHeader(name);
    }

    @Override
    public Header getLastHeader(final String name) {
        return this.headergroup.getLastHeader(name);
    }

    @Override
    public Header[] getAllHeaders() {
        return this.headergroup.getAllHeaders();
    }

    @Override
    public void addHeader(final Header header) {
        
    }

    @Override
    public void addHeader(final String name, final String value) {
        
    }

    @Override
    public void setHeader(final Header header) {
        
    }

    @Override
    public void setHeader(final String name, final String value) {
        
    }

    @Override
    public void setHeaders(final Header[] headers) {
        
    }

    @Override
    public void removeHeader(final Header header) {
        
    }

    @Override
    public void removeHeaders(final String name) {
        
    }

    @Override
    public HeaderIterator headerIterator() {
        return this.headergroup.iterator();
    }

    @Override
    public HeaderIterator headerIterator(final String name) {
        return this.headergroup.iterator(name);
    }

    @Override
    public HttpParams getParams() {
        if (this.params == null) {
            this.params = new BasicHttpParams();
        }
        return this.params;
    }

    @Override
    public void setParams(final HttpParams params) {
        
    }
}
