


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.StringTokenizer;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.ClientCookie;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieRestrictionViolationException;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.cookie.SetCookie2;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class RFC2965PortAttributeHandler implements CookieAttributeHandler {

    public RFC2965PortAttributeHandler() {
        super();
    }

    








    private static int[] parsePortAttribute(final String portValue)
            throws MalformedCookieException {
        final StringTokenizer st = new StringTokenizer(portValue, ",");
        final int[] ports = new int[st.countTokens()];
        try {
            int i = 0;
            while(st.hasMoreTokens()) {
                ports[i] = Integer.parseInt(st.nextToken().trim());
                if (ports[i] < 0) {
                  throw new MalformedCookieException ("Invalid Port attribute.");
                }
                ++i;
            }
        } catch (final NumberFormatException e) {
            throw new MalformedCookieException ("Invalid Port "
                                                + "attribute: " + e.getMessage());
        }
        return ports;
    }

    








    private static boolean portMatch(final int port, final int[] ports) {
        boolean portInList = false;
        for (final int port2 : ports) {
            if (port == port2) {
                portInList = true;
                break;
            }
        }
        return portInList;
    }

    


    public void parse(final SetCookie cookie, final String portValue)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (cookie instanceof SetCookie2) {
            final SetCookie2 cookie2 = (SetCookie2) cookie;
            if (portValue != null && portValue.trim().length() > 0) {
                final int[] ports = parsePortAttribute(portValue);
                cookie2.setPorts(ports);
            }
        }
    }

    



    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        final int port = origin.getPort();
        if (cookie instanceof ClientCookie
                && ((ClientCookie) cookie).containsAttribute(ClientCookie.PORT_ATTR)) {
            if (!portMatch(port, cookie.getPorts())) {
                throw new CookieRestrictionViolationException(
                        "Port attribute violates RFC 2965: "
                        + "Request port not found in cookie's port list.");
            }
        }
    }

    




    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        final int port = origin.getPort();
        if (cookie instanceof ClientCookie
                && ((ClientCookie) cookie).containsAttribute(ClientCookie.PORT_ATTR)) {
            if (cookie.getPorts() == null) {
                
                return false;
            }
            if (!portMatch(port, cookie.getPorts())) {
                return false;
            }
        }
        return true;
    }

}
