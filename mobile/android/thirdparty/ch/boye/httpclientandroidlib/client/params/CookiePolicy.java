


























package ch.boye.httpclientandroidlib.client.params;

import ch.boye.httpclientandroidlib.annotation.Immutable;








@Deprecated
@Immutable
public final class CookiePolicy {

    



    public static final String BROWSER_COMPATIBILITY = "compatibility";

    


    public static final String NETSCAPE = "netscape";

    


    public static final String RFC_2109 = "rfc2109";

    


    public static final String RFC_2965 = "rfc2965";

    


    public static final String BEST_MATCH = "best-match";

    




    public static final String IGNORE_COOKIES = "ignoreCookies";

    private CookiePolicy() {
        super();
    }

}
