

























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Locale;
import java.util.StringTokenizer;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.CookieRestrictionViolationException;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.util.Args;





@Immutable
public class NetscapeDomainHandler extends BasicDomainHandler {

    public NetscapeDomainHandler() {
        super();
    }

    @Override
    public void validate(final Cookie cookie, final CookieOrigin origin)
            throws MalformedCookieException {
        super.validate(cookie, origin);
        
        final String host = origin.getHost();
        final String domain = cookie.getDomain();
        if (host.contains(".")) {
            final int domainParts = new StringTokenizer(domain, ".").countTokens();

            if (isSpecialDomain(domain)) {
                if (domainParts < 2) {
                    throw new CookieRestrictionViolationException("Domain attribute \""
                        + domain
                        + "\" violates the Netscape cookie specification for "
                        + "special domains");
                }
            } else {
                if (domainParts < 3) {
                    throw new CookieRestrictionViolationException("Domain attribute \""
                        + domain
                        + "\" violates the Netscape cookie specification");
                }
            }
        }
    }

   





   private static boolean isSpecialDomain(final String domain) {
       final String ucDomain = domain.toUpperCase(Locale.ENGLISH);
       return ucDomain.endsWith(".COM")
               || ucDomain.endsWith(".EDU")
               || ucDomain.endsWith(".NET")
               || ucDomain.endsWith(".GOV")
               || ucDomain.endsWith(".MIL")
               || ucDomain.endsWith(".ORG")
               || ucDomain.endsWith(".INT");
   }

   @Override
   public boolean match(final Cookie cookie, final CookieOrigin origin) {
       Args.notNull(cookie, "Cookie");
       Args.notNull(origin, "Cookie origin");
       final String host = origin.getHost();
       final String domain = cookie.getDomain();
       if (domain == null) {
           return false;
       }
       return host.endsWith(domain);
   }

}
