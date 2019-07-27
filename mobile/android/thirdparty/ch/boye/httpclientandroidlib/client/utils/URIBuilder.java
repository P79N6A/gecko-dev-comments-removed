

























package ch.boye.httpclientandroidlib.client.utils;

import java.net.URI;
import java.net.URISyntaxException;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import ch.boye.httpclientandroidlib.Consts;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.conn.util.InetAddressUtils;
import ch.boye.httpclientandroidlib.message.BasicNameValuePair;






@NotThreadSafe
public class URIBuilder {

    private String scheme;
    private String encodedSchemeSpecificPart;
    private String encodedAuthority;
    private String userInfo;
    private String encodedUserInfo;
    private String host;
    private int port;
    private String path;
    private String encodedPath;
    private String encodedQuery;
    private List<NameValuePair> queryParams;
    private String query;
    private String fragment;
    private String encodedFragment;

    


    public URIBuilder() {
        super();
        this.port = -1;
    }

    





    public URIBuilder(final String string) throws URISyntaxException {
        super();
        digestURI(new URI(string));
    }

    



    public URIBuilder(final URI uri) {
        super();
        digestURI(uri);
    }

    private List <NameValuePair> parseQuery(final String query, final Charset charset) {
        if (query != null && query.length() > 0) {
            return URLEncodedUtils.parse(query, charset);
        }
        return null;
    }

    


    public URI build() throws URISyntaxException {
        return new URI(buildString());
    }

    private String buildString() {
        final StringBuilder sb = new StringBuilder();
        if (this.scheme != null) {
            sb.append(this.scheme).append(':');
        }
        if (this.encodedSchemeSpecificPart != null) {
            sb.append(this.encodedSchemeSpecificPart);
        } else {
            if (this.encodedAuthority != null) {
                sb.append("//").append(this.encodedAuthority);
            } else if (this.host != null) {
                sb.append("//");
                if (this.encodedUserInfo != null) {
                    sb.append(this.encodedUserInfo).append("@");
                } else if (this.userInfo != null) {
                    sb.append(encodeUserInfo(this.userInfo)).append("@");
                }
                if (InetAddressUtils.isIPv6Address(this.host)) {
                    sb.append("[").append(this.host).append("]");
                } else {
                    sb.append(this.host);
                }
                if (this.port >= 0) {
                    sb.append(":").append(this.port);
                }
            }
            if (this.encodedPath != null) {
                sb.append(normalizePath(this.encodedPath));
            } else if (this.path != null) {
                sb.append(encodePath(normalizePath(this.path)));
            }
            if (this.encodedQuery != null) {
                sb.append("?").append(this.encodedQuery);
            } else if (this.queryParams != null) {
                sb.append("?").append(encodeUrlForm(this.queryParams));
            } else if (this.query != null) {
                sb.append("?").append(encodeUric(this.query));
            }
        }
        if (this.encodedFragment != null) {
            sb.append("#").append(this.encodedFragment);
        } else if (this.fragment != null) {
            sb.append("#").append(encodeUric(this.fragment));
        }
        return sb.toString();
    }

    private void digestURI(final URI uri) {
        this.scheme = uri.getScheme();
        this.encodedSchemeSpecificPart = uri.getRawSchemeSpecificPart();
        this.encodedAuthority = uri.getRawAuthority();
        this.host = uri.getHost();
        this.port = uri.getPort();
        this.encodedUserInfo = uri.getRawUserInfo();
        this.userInfo = uri.getUserInfo();
        this.encodedPath = uri.getRawPath();
        this.path = uri.getPath();
        this.encodedQuery = uri.getRawQuery();
        this.queryParams = parseQuery(uri.getRawQuery(), Consts.UTF_8);
        this.encodedFragment = uri.getRawFragment();
        this.fragment = uri.getFragment();
    }

    private String encodeUserInfo(final String userInfo) {
        return URLEncodedUtils.encUserInfo(userInfo, Consts.UTF_8);
    }

    private String encodePath(final String path) {
        return URLEncodedUtils.encPath(path, Consts.UTF_8);
    }

    private String encodeUrlForm(final List<NameValuePair> params) {
        return URLEncodedUtils.format(params, Consts.UTF_8);
    }

    private String encodeUric(final String fragment) {
        return URLEncodedUtils.encUric(fragment, Consts.UTF_8);
    }

    


    public URIBuilder setScheme(final String scheme) {
        this.scheme = scheme;
        return this;
    }

    



    public URIBuilder setUserInfo(final String userInfo) {
        this.userInfo = userInfo;
        this.encodedSchemeSpecificPart = null;
        this.encodedAuthority = null;
        this.encodedUserInfo = null;
        return this;
    }

    



    public URIBuilder setUserInfo(final String username, final String password) {
        return setUserInfo(username + ':' + password);
    }

    


    public URIBuilder setHost(final String host) {
        this.host = host;
        this.encodedSchemeSpecificPart = null;
        this.encodedAuthority = null;
        return this;
    }

    


    public URIBuilder setPort(final int port) {
        this.port = port < 0 ? -1 : port;
        this.encodedSchemeSpecificPart = null;
        this.encodedAuthority = null;
        return this;
    }

    


    public URIBuilder setPath(final String path) {
        this.path = path;
        this.encodedSchemeSpecificPart = null;
        this.encodedPath = null;
        return this;
    }

    


    public URIBuilder removeQuery() {
        this.queryParams = null;
        this.query = null;
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        return this;
    }

    








    @Deprecated
    public URIBuilder setQuery(final String query) {
        this.queryParams = parseQuery(query, Consts.UTF_8);
        this.query = null;
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        return this;
    }

    








    public URIBuilder setParameters(final List <NameValuePair> nvps) {
        if (this.queryParams == null) {
            this.queryParams = new ArrayList<NameValuePair>();
        } else {
            this.queryParams.clear();
        }
        this.queryParams.addAll(nvps);
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.query = null;
        return this;
    }

    








    public URIBuilder addParameters(final List <NameValuePair> nvps) {
        if (this.queryParams == null) {
            this.queryParams = new ArrayList<NameValuePair>();
        }
        this.queryParams.addAll(nvps);
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.query = null;
        return this;
    }

    








    public URIBuilder setParameters(final NameValuePair... nvps) {
        if (this.queryParams == null) {
            this.queryParams = new ArrayList<NameValuePair>();
        } else {
            this.queryParams.clear();
        }
        for (final NameValuePair nvp: nvps) {
            this.queryParams.add(nvp);
        }
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.query = null;
        return this;
    }

    






    public URIBuilder addParameter(final String param, final String value) {
        if (this.queryParams == null) {
            this.queryParams = new ArrayList<NameValuePair>();
        }
        this.queryParams.add(new BasicNameValuePair(param, value));
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.query = null;
        return this;
    }

    






    public URIBuilder setParameter(final String param, final String value) {
        if (this.queryParams == null) {
            this.queryParams = new ArrayList<NameValuePair>();
        }
        if (!this.queryParams.isEmpty()) {
            for (final Iterator<NameValuePair> it = this.queryParams.iterator(); it.hasNext(); ) {
                final NameValuePair nvp = it.next();
                if (nvp.getName().equals(param)) {
                    it.remove();
                }
            }
        }
        this.queryParams.add(new BasicNameValuePair(param, value));
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.query = null;
        return this;
    }

    




    public URIBuilder clearParameters() {
        this.queryParams = null;
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        return this;
    }

    








    public URIBuilder setCustomQuery(final String query) {
        this.query = query;
        this.encodedQuery = null;
        this.encodedSchemeSpecificPart = null;
        this.queryParams = null;
        return this;
    }

    



    public URIBuilder setFragment(final String fragment) {
        this.fragment = fragment;
        this.encodedFragment = null;
        return this;
    }

    


    public boolean isAbsolute() {
        return this.scheme != null;
    }

    


    public boolean isOpaque() {
        return this.path == null;
    }

    public String getScheme() {
        return this.scheme;
    }

    public String getUserInfo() {
        return this.userInfo;
    }

    public String getHost() {
        return this.host;
    }

    public int getPort() {
        return this.port;
    }

    public String getPath() {
        return this.path;
    }

    public List<NameValuePair> getQueryParams() {
        if (this.queryParams != null) {
            return new ArrayList<NameValuePair>(this.queryParams);
        } else {
            return new ArrayList<NameValuePair>();
        }
    }

    public String getFragment() {
        return this.fragment;
    }

    @Override
    public String toString() {
        return buildString();
    }

    private static String normalizePath(final String path) {
        String s = path;
        if (s == null) {
            return null;
        }
        int n = 0;
        for (; n < s.length(); n++) {
            if (s.charAt(n) != '/') {
                break;
            }
        }
        if (n > 1) {
            s = s.substring(n - 1);
        }
        return s;
    }

}
