


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.ArrayList;
import java.util.List;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.ClientCookie;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SM;
import ch.boye.httpclientandroidlib.message.BufferedHeader;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;









@NotThreadSafe 
public class NetscapeDraftSpec extends CookieSpecBase {

    protected static final String EXPIRES_PATTERN = "EEE, dd-MMM-yy HH:mm:ss z";

    private final String[] datepatterns;

    
    public NetscapeDraftSpec(final String[] datepatterns) {
        super();
        if (datepatterns != null) {
            this.datepatterns = datepatterns.clone();
        } else {
            this.datepatterns = new String[] { EXPIRES_PATTERN };
        }
        registerAttribHandler(ClientCookie.PATH_ATTR, new BasicPathHandler());
        registerAttribHandler(ClientCookie.DOMAIN_ATTR, new NetscapeDomainHandler());
        registerAttribHandler(ClientCookie.MAX_AGE_ATTR, new BasicMaxAgeHandler());
        registerAttribHandler(ClientCookie.SECURE_ATTR, new BasicSecureHandler());
        registerAttribHandler(ClientCookie.COMMENT_ATTR, new BasicCommentHandler());
        registerAttribHandler(ClientCookie.EXPIRES_ATTR, new BasicExpiresHandler(
                this.datepatterns));
    }

    
    public NetscapeDraftSpec() {
        this(null);
    }

    























    public List<Cookie> parse(final Header header, final CookieOrigin origin)
            throws MalformedCookieException {
        Args.notNull(header, "Header");
        Args.notNull(origin, "Cookie origin");
        if (!header.getName().equalsIgnoreCase(SM.SET_COOKIE)) {
            throw new MalformedCookieException("Unrecognized cookie header '"
                    + header.toString() + "'");
        }
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
        return parse(new HeaderElement[] { parser.parseHeader(buffer, cursor) }, origin);
    }

    public List<Header> formatCookies(final List<Cookie> cookies) {
        Args.notEmpty(cookies, "List of cookies");
        final CharArrayBuffer buffer = new CharArrayBuffer(20 * cookies.size());
        buffer.append(SM.COOKIE);
        buffer.append(": ");
        for (int i = 0; i < cookies.size(); i++) {
            final Cookie cookie = cookies.get(i);
            if (i > 0) {
                buffer.append("; ");
            }
            buffer.append(cookie.getName());
            final String s = cookie.getValue();
            if (s != null) {
                buffer.append("=");
                buffer.append(s);
            }
        }
        final List<Header> headers = new ArrayList<Header>(1);
        headers.add(new BufferedHeader(buffer));
        return headers;
    }

    public int getVersion() {
        return 0;
    }

    public Header getVersionHeader() {
        return null;
    }

    @Override
    public String toString() {
        return "netscape";
    }

}
