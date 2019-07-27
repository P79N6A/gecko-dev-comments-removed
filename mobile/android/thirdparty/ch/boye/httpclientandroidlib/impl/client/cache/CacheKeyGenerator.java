

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URL;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.cache.HeaderConstants;
import ch.boye.httpclientandroidlib.client.cache.HttpCacheEntry;
import ch.boye.httpclientandroidlib.client.utils.URIUtils;




@Immutable
class CacheKeyGenerator {

    private static final URI BASE_URI = URI.create("http://example.com/");

    







    public String getURI(final HttpHost host, final HttpRequest req) {
        if (isRelativeRequest(req)) {
            return canonicalizeUri(String.format("%s%s", host.toString(), req.getRequestLine().getUri()));
        }
        return canonicalizeUri(req.getRequestLine().getUri());
    }

    public String canonicalizeUri(final String uri) {
        try {
            final URI normalized = URIUtils.resolve(BASE_URI, uri);
            final URL u = new URL(normalized.toASCIIString());
            final String protocol = u.getProtocol();
            final String hostname = u.getHost();
            final int port = canonicalizePort(u.getPort(), protocol);
            final String path = u.getPath();
            final String query = u.getQuery();
            final String file = (query != null) ? (path + "?" + query) : path;
            final URL out = new URL(protocol, hostname, port, file);
            return out.toString();
        } catch (final IllegalArgumentException e) {
            return uri;
        } catch (final MalformedURLException e) {
            return uri;
        }
    }

    private int canonicalizePort(final int port, final String protocol) {
        if (port == -1 && "http".equalsIgnoreCase(protocol)) {
            return 80;
        } else if (port == -1 && "https".equalsIgnoreCase(protocol)) {
            return 443;
        }
        return port;
    }

    private boolean isRelativeRequest(final HttpRequest req) {
        final String requestUri = req.getRequestLine().getUri();
        return ("*".equals(requestUri) || requestUri.startsWith("/"));
    }

    protected String getFullHeaderValue(final Header[] headers) {
        if (headers == null) {
            return "";
        }

        final StringBuilder buf = new StringBuilder("");
        boolean first = true;
        for (final Header hdr : headers) {
            if (!first) {
                buf.append(", ");
            }
            buf.append(hdr.getValue().trim());
            first = false;

        }
        return buf.toString();
    }

    









    public String getVariantURI(final HttpHost host, final HttpRequest req, final HttpCacheEntry entry) {
        if (!entry.hasVariants()) {
            return getURI(host, req);
        }
        return getVariantKey(req, entry) + getURI(host, req);
    }

    








    public String getVariantKey(final HttpRequest req, final HttpCacheEntry entry) {
        final List<String> variantHeaderNames = new ArrayList<String>();
        for (final Header varyHdr : entry.getHeaders(HeaderConstants.VARY)) {
            for (final HeaderElement elt : varyHdr.getElements()) {
                variantHeaderNames.add(elt.getName());
            }
        }
        Collections.sort(variantHeaderNames);

        StringBuilder buf;
        try {
            buf = new StringBuilder("{");
            boolean first = true;
            for (final String headerName : variantHeaderNames) {
                if (!first) {
                    buf.append("&");
                }
                buf.append(URLEncoder.encode(headerName, Consts.UTF_8.name()));
                buf.append("=");
                buf.append(URLEncoder.encode(getFullHeaderValue(req.getHeaders(headerName)),
                        Consts.UTF_8.name()));
                first = false;
            }
            buf.append("}");
        } catch (final UnsupportedEncodingException uee) {
            throw new RuntimeException("couldn't encode to UTF-8", uee);
        }
        return buf.toString();
    }

}
