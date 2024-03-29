

























package ch.boye.httpclientandroidlib.client.utils;

import java.net.URI;
import java.net.URISyntaxException;
import java.util.List;
import java.util.Locale;
import java.util.Stack;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.TextUtils;







@Immutable
public class URIUtils {

     



























    @Deprecated
    public static URI createURI(
            final String scheme,
            final String host,
            final int port,
            final String path,
            final String query,
            final String fragment) throws URISyntaxException {
        final StringBuilder buffer = new StringBuilder();
        if (host != null) {
            if (scheme != null) {
                buffer.append(scheme);
                buffer.append("://");
            }
            buffer.append(host);
            if (port > 0) {
                buffer.append(':');
                buffer.append(port);
            }
        }
        if (path == null || !path.startsWith("/")) {
            buffer.append('/');
        }
        if (path != null) {
            buffer.append(path);
        }
        if (query != null) {
            buffer.append('?');
            buffer.append(query);
        }
        if (fragment != null) {
            buffer.append('#');
            buffer.append(fragment);
        }
        return new URI(buffer.toString());
    }

    















    public static URI rewriteURI(
            final URI uri,
            final HttpHost target,
            final boolean dropFragment) throws URISyntaxException {
        Args.notNull(uri, "URI");
        if (uri.isOpaque()) {
            return uri;
        }
        final URIBuilder uribuilder = new URIBuilder(uri);
        if (target != null) {
            uribuilder.setScheme(target.getSchemeName());
            uribuilder.setHost(target.getHostName());
            uribuilder.setPort(target.getPort());
        } else {
            uribuilder.setScheme(null);
            uribuilder.setHost(null);
            uribuilder.setPort(-1);
        }
        if (dropFragment) {
            uribuilder.setFragment(null);
        }
        if (TextUtils.isEmpty(uribuilder.getPath())) {
            uribuilder.setPath("/");
        }
        return uribuilder.build();
    }

    




    public static URI rewriteURI(
            final URI uri,
            final HttpHost target) throws URISyntaxException {
        return rewriteURI(uri, target, false);
    }

    










    public static URI rewriteURI(final URI uri) throws URISyntaxException {
        Args.notNull(uri, "URI");
        if (uri.isOpaque()) {
            return uri;
        }
        final URIBuilder uribuilder = new URIBuilder(uri);
        if (uribuilder.getUserInfo() != null) {
            uribuilder.setUserInfo(null);
        }
        if (TextUtils.isEmpty(uribuilder.getPath())) {
            uribuilder.setPath("/");
        }
        if (uribuilder.getHost() != null) {
            uribuilder.setHost(uribuilder.getHost().toLowerCase(Locale.ENGLISH));
        }
        uribuilder.setFragment(null);
        return uribuilder.build();
    }

    







    public static URI resolve(final URI baseURI, final String reference) {
        return URIUtils.resolve(baseURI, URI.create(reference));
    }

    







    public static URI resolve(final URI baseURI, final URI reference){
        Args.notNull(baseURI, "Base URI");
        Args.notNull(reference, "Reference URI");
        URI ref = reference;
        final String s = ref.toString();
        if (s.startsWith("?")) {
            return resolveReferenceStartingWithQueryString(baseURI, ref);
        }
        final boolean emptyReference = s.length() == 0;
        if (emptyReference) {
            ref = URI.create("#");
        }
        URI resolved = baseURI.resolve(ref);
        if (emptyReference) {
            final String resolvedString = resolved.toString();
            resolved = URI.create(resolvedString.substring(0,
                resolvedString.indexOf('#')));
        }
        return normalizeSyntax(resolved);
    }

    






    private static URI resolveReferenceStartingWithQueryString(
            final URI baseURI, final URI reference) {
        String baseUri = baseURI.toString();
        baseUri = baseUri.indexOf('?') > -1 ?
            baseUri.substring(0, baseUri.indexOf('?')) : baseUri;
        return URI.create(baseUri + reference.toString());
    }

    






    private static URI normalizeSyntax(final URI uri) {
        if (uri.isOpaque() || uri.getAuthority() == null) {
            
            return uri;
        }
        Args.check(uri.isAbsolute(), "Base URI must be absolute");
        final String path = uri.getPath() == null ? "" : uri.getPath();
        final String[] inputSegments = path.split("/");
        final Stack<String> outputSegments = new Stack<String>();
        for (final String inputSegment : inputSegments) {
            if ((inputSegment.length() == 0)
                || (".".equals(inputSegment))) {
                
            } else if ("..".equals(inputSegment)) {
                if (!outputSegments.isEmpty()) {
                    outputSegments.pop();
                }
            } else {
                outputSegments.push(inputSegment);
            }
        }
        final StringBuilder outputBuffer = new StringBuilder();
        for (final String outputSegment : outputSegments) {
            outputBuffer.append('/').append(outputSegment);
        }
        if (path.lastIndexOf('/') == path.length() - 1) {
            
            outputBuffer.append('/');
        }
        try {
            final String scheme = uri.getScheme().toLowerCase(Locale.ENGLISH);
            final String auth = uri.getAuthority().toLowerCase(Locale.ENGLISH);
            final URI ref = new URI(scheme, auth, outputBuffer.toString(),
                    null, null);
            if (uri.getQuery() == null && uri.getFragment() == null) {
                return ref;
            }
            final StringBuilder normalized = new StringBuilder(
                    ref.toASCIIString());
            if (uri.getQuery() != null) {
                
                normalized.append('?').append(uri.getRawQuery());
            }
            if (uri.getFragment() != null) {
                
                normalized.append('#').append(uri.getRawFragment());
            }
            return URI.create(normalized.toString());
        } catch (final URISyntaxException e) {
            throw new IllegalArgumentException(e);
        }
    }

    








    public static HttpHost extractHost(final URI uri) {
        if (uri == null) {
            return null;
        }
        HttpHost target = null;
        if (uri.isAbsolute()) {
            int port = uri.getPort(); 
            String host = uri.getHost();
            if (host == null) { 
                
                host = uri.getAuthority();
                if (host != null) {
                    
                    final int at = host.indexOf('@');
                    if (at >= 0) {
                        if (host.length() > at+1 ) {
                            host = host.substring(at+1);
                        } else {
                            host = null; 
                        }
                    }
                    
                    if (host != null) {
                        final int colon = host.indexOf(':');
                        if (colon >= 0) {
                            final int pos = colon + 1;
                            int len = 0;
                            for (int i = pos; i < host.length(); i++) {
                                if (Character.isDigit(host.charAt(i))) {
                                    len++;
                                } else {
                                    break;
                                }
                            }
                            if (len > 0) {
                                try {
                                    port = Integer.parseInt(host.substring(pos, pos + len));
                                } catch (final NumberFormatException ex) {
                                }
                            }
                            host = host.substring(0, colon);
                        }
                    }
                }
            }
            final String scheme = uri.getScheme();
            if (!TextUtils.isBlank(host)) {
                target = new HttpHost(host, port, scheme);
            }
        }
        return target;
    }

    















    public static URI resolve(
            final URI originalURI,
            final HttpHost target,
            final List<URI> redirects) throws URISyntaxException {
        Args.notNull(originalURI, "Request URI");
        final URIBuilder uribuilder;
        if (redirects == null || redirects.isEmpty()) {
            uribuilder = new URIBuilder(originalURI);
        } else {
            uribuilder = new URIBuilder(redirects.get(redirects.size() - 1));
            String frag = uribuilder.getFragment();
            
            for (int i = redirects.size() - 1; frag == null && i >= 0; i--) {
                frag = redirects.get(i).getFragment();
            }
            uribuilder.setFragment(frag);
        }
        
        if (uribuilder.getFragment() == null) {
            uribuilder.setFragment(originalURI.getFragment());
        }
        
        if (target != null && !uribuilder.isAbsolute()) {
            uribuilder.setScheme(target.getSchemeName());
            uribuilder.setHost(target.getHostName());
            uribuilder.setPort(target.getPort());
        }
        return uribuilder.build();
    }

    


    private URIUtils() {
    }

}
