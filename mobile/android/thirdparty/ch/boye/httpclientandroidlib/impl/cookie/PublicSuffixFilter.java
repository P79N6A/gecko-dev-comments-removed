






























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import ch.boye.httpclientandroidlib.client.utils.Punycode;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieOrigin;
import ch.boye.httpclientandroidlib.cookie.MalformedCookieException;
import ch.boye.httpclientandroidlib.cookie.SetCookie;











public class PublicSuffixFilter implements CookieAttributeHandler {
    private final CookieAttributeHandler wrapped;
    private Set<String> exceptions;
    private Set<String> suffixes;

    public PublicSuffixFilter(CookieAttributeHandler wrapped) {
        this.wrapped = wrapped;
    }

    





    public void setPublicSuffixes(Collection<String> suffixes) {
        this.suffixes = new HashSet<String>(suffixes);
    }

    




    public void setExceptions(Collection<String> exceptions) {
        this.exceptions = new HashSet<String>(exceptions);
    }

    


    public boolean match(Cookie cookie, CookieOrigin origin) {
        if (isForPublicSuffix(cookie)) return false;
        return wrapped.match(cookie, origin);
    }

    public void parse(SetCookie cookie, String value) throws MalformedCookieException {
        wrapped.parse(cookie, value);
    }

    public void validate(Cookie cookie, CookieOrigin origin) throws MalformedCookieException {
        wrapped.validate(cookie, origin);
    }

    private boolean isForPublicSuffix(Cookie cookie) {
        String domain = cookie.getDomain();
        if (domain.startsWith(".")) domain = domain.substring(1);
        domain = Punycode.toUnicode(domain);

        
        if (this.exceptions != null) {
            if (this.exceptions.contains(domain)) return false;
        }


        if (this.suffixes == null) return false;

        do {
            if (this.suffixes.contains(domain)) return true;
            
            if (domain.startsWith("*.")) domain = domain.substring(2);
            int nextdot = domain.indexOf('.');
            if (nextdot == -1) break;
            domain = "*" + domain.substring(nextdot);
        } while (domain.length() > 0);

        return false;
    }
}
