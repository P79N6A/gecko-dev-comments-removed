


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Locale;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.ClientCookie;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieRestrictionViolationException;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class RFC2965DomainAttributeHandler implements CookieAttributeHandler {

    public RFC2965DomainAttributeHandler() {
        super();
    }

    


    public void parse(
            final SetCookie cookie, final String domain) throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        if (domain == null) {
            throw new MalformedCookieException(
                    "Missing value for domain attribute");
        }
        if (domain.trim().length() == 0) {
            throw new MalformedCookieException(
                    "Blank value for domain attribute");
        }
        String s = domain;
        s = s.toLowerCase(Locale.ENGLISH);
        if (!domain.startsWith(".")) {
            
            
            
            
            
            s = '.' + s;
        }
        cookie.setDomain(s);
    }

    














    public boolean domainMatch(final String host, final String domain) {
        final boolean match = host.equals(domain)
                        || (domain.startsWith(".") && host.endsWith(domain));

        return match;
    }

    


    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        final String host = origin.getHost().toLowerCase(Locale.ENGLISH);
        if (cookie.getDomain() == null) {
            throw new CookieRestrictionViolationException("Invalid cookie state: " +
                                               "domain not specified");
        }
        final String cookieDomain = cookie.getDomain().toLowerCase(Locale.ENGLISH);

        if (cookie instanceof ClientCookie
                && ((ClientCookie) cookie).containsAttribute(ClientCookie.DOMAIN_ATTR)) {
            
            if (!cookieDomain.startsWith(".")) {
                throw new CookieRestrictionViolationException("Domain attribute \"" +
                    cookie.getDomain() + "\" violates RFC 2109: domain must start with a dot");
            }

            
            
            final int dotIndex = cookieDomain.indexOf('.', 1);
            if (((dotIndex < 0) || (dotIndex == cookieDomain.length() - 1))
                && (!cookieDomain.equals(".local"))) {
                throw new CookieRestrictionViolationException(
                        "Domain attribute \"" + cookie.getDomain()
                        + "\" violates RFC 2965: the value contains no embedded dots "
                        + "and the value is not .local");
            }

            
            if (!domainMatch(host, cookieDomain)) {
                throw new CookieRestrictionViolationException(
                        "Domain attribute \"" + cookie.getDomain()
                        + "\" violates RFC 2965: effective host name does not "
                        + "domain-match domain attribute.");
            }

            
            final String effectiveHostWithoutDomain = host.substring(
                    0, host.length() - cookieDomain.length());
            if (effectiveHostWithoutDomain.indexOf('.') != -1) {
                throw new CookieRestrictionViolationException("Domain attribute \""
                                                   + cookie.getDomain() + "\" violates RFC 2965: "
                                                   + "effective host minus domain may not contain any dots");
            }
        } else {
            
            
            if (!cookie.getDomain().equals(host)) {
                throw new CookieRestrictionViolationException("Illegal domain attribute: \""
                                                   + cookie.getDomain() + "\"."
                                                   + "Domain of origin: \""
                                                   + host + "\"");
            }
        }
    }

    


    public boolean match(final Cookie cookie, final CookieOrigin origin) {
        Args.notNull(cookie, "Cookie");
        Args.notNull(origin, "Cookie origin");
        final String host = origin.getHost().toLowerCase(Locale.ENGLISH);
        final String cookieDomain = cookie.getDomain();

        
        
        if (!domainMatch(host, cookieDomain)) {
            return false;
        }
        
        final String effectiveHostWithoutDomain = host.substring(
                0, host.length() - cookieDomain.length());
        return effectiveHostWithoutDomain.indexOf('.') == -1;
    }

}
