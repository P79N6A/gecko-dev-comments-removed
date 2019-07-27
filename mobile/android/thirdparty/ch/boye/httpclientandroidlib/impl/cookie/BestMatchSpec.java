


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.List;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SM;
import ch.boye.httpclientandroidlib.cookie.SetCookie2;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;







@NotThreadSafe 
public class BestMatchSpec implements CookieSpec {

    private final String[] datepatterns;
    private final boolean oneHeader;

    
    private RFC2965Spec strict; 
    private RFC2109Spec obsoleteStrict; 
    private BrowserCompatSpec compat; 

    public BestMatchSpec(final String[] datepatterns, final boolean oneHeader) {
        super();
        this.datepatterns = datepatterns == null ? null : datepatterns.clone();
        this.oneHeader = oneHeader;
    }

    public BestMatchSpec() {
        this(null, false);
    }

    private RFC2965Spec getStrict() {
        if (this.strict == null) {
             this.strict = new RFC2965Spec(this.datepatterns, this.oneHeader);
        }
        return strict;
    }

    private RFC2109Spec getObsoleteStrict() {
        if (this.obsoleteStrict == null) {
             this.obsoleteStrict = new RFC2109Spec(this.datepatterns, this.oneHeader);
        }
        return obsoleteStrict;
    }

    private BrowserCompatSpec getCompat() {
        if (this.compat == null) {
            this.compat = new BrowserCompatSpec(this.datepatterns);
        }
        return compat;
    }

    public List<Cookie> parse(
            final Header header,
            final CookieOrigin origin) throws MalformedCookieException {
        Args.notNull(header, "Header");
        Args.notNull(origin, "Cookie origin");
        HeaderElement[] helems = header.getElements();
        boolean versioned = false;
        boolean netscape = false;
        for (final HeaderElement helem: helems) {
            if (helem.getParameterByName("version") != null) {
                versioned = true;
            }
            if (helem.getParameterByName("expires") != null) {
               netscape = true;
            }
        }
        if (netscape || !versioned) {
            
            
            final NetscapeDraftHeaderParser parser = NetscapeDraftHeaderParser.DEFAULT;
            final CharArrayBuffer buffer;
            final ParserCursor cursor;
            if (header instanceof FormattedHeader) {
                buffer = ((FormattedHeader) header).getBuffer();
                cursor = new ParserCursor(
                        ((FormattedHeader) header).getValuePos(),
                        buffer.length());
            } else {
                final String s = header.getValue();
                if (s == null) {
                    throw new MalformedCookieException("Header value is null");
                }
                buffer = new CharArrayBuffer(s.length());
                buffer.append(s);
                cursor = new ParserCursor(0, buffer.length());
            }
            helems = new HeaderElement[] { parser.parseHeader(buffer, cursor) };
            return getCompat().parse(helems, origin);
        } else {
            if (SM.SET_COOKIE2.equals(header.getName())) {
                return getStrict().parse(helems, origin);
            } else {
                return getObsoleteStrict().parse(helems, origin);
            }
        }
    }

    public void validate(
            final Cookie cookie,
            final CookieOrigin origin) throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        if (cookie.getVersion() > 0) {
            if (cookie instanceof SetCookie2) {
                getStrict().validate(cookie, origin);
            } else {
                getObsoleteStrict().validate(cookie, origin);
            }
        } else {
            getCompat().validate(cookie, origin);
        }
    }

    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        if (cookie.getVersion() > 0) {
            if (cookie instanceof SetCookie2) {
                return getStrict().match(cookie, origin);
            } else {
                return getObsoleteStrict().match(cookie, origin);
            }
        } else {
            return getCompat().match(cookie, origin);
        }
    }

    public List<Header> formatCookies(final List<Cookie> cookies) {
        Args.notNull(cookies, "List of cookies");
        int version = Integer.MAX_VALUE;
        boolean isSetCookie2 = true;
        for (final Cookie cookie: cookies) {
            if (!(cookie instanceof SetCookie2)) {
                isSetCookie2 = false;
            }
            if (cookie.getVersion() < version) {
                version = cookie.getVersion();
            }
        }
        if (version > 0) {
            if (isSetCookie2) {
                return getStrict().formatCookies(cookies);
            } else {
                return getObsoleteStrict().formatCookies(cookies);
            }
        } else {
            return getCompat().formatCookies(cookies);
        }
    }

    public int getVersion() {
        return getStrict().getVersion();
    }

    public Header getVersionHeader() {
        return getStrict().getVersionHeader();
    }

    @Override
    public String toString() {
        return "best-match";
    }

}
