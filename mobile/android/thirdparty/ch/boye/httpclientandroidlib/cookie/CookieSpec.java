


























package ch.boye.httpclientandroidlib.cookie;

import java.util.List;

import ch.boye.httpclientandroidlib.Header;














public interface CookieSpec {

    





    int getVersion();

    












    List<Cookie> parse(Header header, CookieOrigin origin) throws MalformedCookieException;

    







    void validate(Cookie cookie, CookieOrigin origin) throws MalformedCookieException;

    








    boolean match(Cookie cookie, CookieOrigin origin);

    






    List<Header> formatCookies(List<Cookie> cookies);

    




    Header getVersionHeader();

}
