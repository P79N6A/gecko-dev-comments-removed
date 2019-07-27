

























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.ClientCookie;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SM;
import ch.boye.httpclientandroidlib.message.BufferedHeader;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;






@NotThreadSafe 
public class RFC2965Spec extends RFC2109Spec {

    



    public RFC2965Spec() {
        this(null, false);
    }

    public RFC2965Spec(final String[] datepatterns, final boolean oneHeader) {
        super(datepatterns, oneHeader);
        registerAttribHandler(ClientCookie.DOMAIN_ATTR, new RFC2965DomainAttributeHandler());
        registerAttribHandler(ClientCookie.PORT_ATTR, new RFC2965PortAttributeHandler());
        registerAttribHandler(ClientCookie.COMMENTURL_ATTR, new RFC2965CommentUrlAttributeHandler());
        registerAttribHandler(ClientCookie.DISCARD_ATTR, new RFC2965DiscardAttributeHandler());
        registerAttribHandler(ClientCookie.VERSION_ATTR, new RFC2965VersionAttributeHandler());
    }

    @Override
    public List<Cookie> parse(
            final Header header,
            final CookieOrigin origin) throws MalformedCookieException {
        Args.notNull(header, "Header");
        Args.notNull(origin, "Cookie origin");
        if (!header.getName().equalsIgnoreCase(SM.SET_COOKIE2)) {
            throw new MalformedCookieException("Unrecognized cookie header '"
                    + header.toString() + "'");
        }
        final HeaderElement[] elems = header.getElements();
        return createCookies(elems, adjustEffectiveHost(origin));
    }

    @Override
    protected List<Cookie> parse(
            final HeaderElement[] elems,
            final CookieOrigin origin) throws MalformedCookieException {
        return createCookies(elems, adjustEffectiveHost(origin));
    }

    private List<Cookie> createCookies(
            final HeaderElement[] elems,
            final CookieOrigin origin) throws MalformedCookieException {
        final List<Cookie> cookies = new ArrayList<Cookie>(elems.length);
        for (final HeaderElement headerelement : elems) {
            final String name = headerelement.getName();
            final String value = headerelement.getValue();
            if (name == null || name.length() == 0) {
                throw new MalformedCookieException("Cookie name may not be empty");
            }

            final BasicClientCookie2 cookie = new BasicClientCookie2(name, value);
            cookie.setPath(getDefaultPath(origin));
            cookie.setDomain(getDefaultDomain(origin));
            cookie.setPorts(new int [] { origin.getPort() });
            
            final NameValuePair[] attribs = headerelement.getParameters();

            
            
            final Map<String, NameValuePair> attribmap =
                    new HashMap<String, NameValuePair>(attribs.length);
            for (int j = attribs.length - 1; j >= 0; j--) {
                final NameValuePair param = attribs[j];
                attribmap.put(param.getName().toLowerCase(Locale.ENGLISH), param);
            }
            for (final Map.Entry<String, NameValuePair> entry : attribmap.entrySet()) {
                final NameValuePair attrib = entry.getValue();
                final String s = attrib.getName().toLowerCase(Locale.ENGLISH);

                cookie.setAttribute(s, attrib.getValue());

                final CookieAttributeHandler handler = findAttribHandler(s);
                if (handler != null) {
                    handler.parse(cookie, attrib.getValue());
                }
            }
            cookies.add(cookie);
        }
        return cookies;
    }

    @Override
    public void validate(
            final Cookie cookie, final CookieOrigin origin) throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        super.validate(cookie, adjustEffectiveHost(origin));
    }

    @Override
    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        return super.match(cookie, adjustEffectiveHost(origin));
    }

    


    @Override
    protected void formatCookieAsVer(final CharArrayBuffer buffer,
            final Cookie cookie, final int version) {
        super.formatCookieAsVer(buffer, cookie, version);
        
        if (cookie instanceof ClientCookie) {
            
            final String s = ((ClientCookie) cookie).getAttribute(ClientCookie.PORT_ATTR);
            if (s != null) {
                buffer.append("; $Port");
                buffer.append("=\"");
                if (s.trim().length() > 0) {
                    final int[] ports = cookie.getPorts();
                    if (ports != null) {
                        final int len = ports.length;
                        for (int i = 0; i < len; i++) {
                            if (i > 0) {
                                buffer.append(",");
                            }
                            buffer.append(Integer.toString(ports[i]));
                        }
                    }
                }
                buffer.append("\"");
            }
        }
    }

    









    private static CookieOrigin adjustEffectiveHost(final CookieOrigin origin) {
        String host = origin.getHost();

        
        
        boolean isLocalHost = true;
        for (int i = 0; i < host.length(); i++) {
            final char ch = host.charAt(i);
            if (ch == '.' || ch == ':') {
                isLocalHost = false;
                break;
            }
        }
        if (isLocalHost) {
            host += ".local";
            return new CookieOrigin(
                    host,
                    origin.getPort(),
                    origin.getPath(),
                    origin.isSecure());
        } else {
            return origin;
        }
    }

    @Override
    public int getVersion() {
        return 1;
    }

    @Override
    public Header getVersionHeader() {
        final CharArrayBuffer buffer = new CharArrayBuffer(40);
        buffer.append(SM.COOKIE2);
        buffer.append(": ");
        buffer.append("$Version=");
        buffer.append(Integer.toString(getVersion()));
        return new BufferedHeader(buffer);
    }

    @Override
    public String toString() {
        return "rfc2965";
    }

}

