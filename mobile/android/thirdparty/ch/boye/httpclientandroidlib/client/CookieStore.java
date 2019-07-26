

























package ch.boye.httpclientandroidlib.client;

import java.util.Date;
import java.util.List;

import ch.boye.httpclientandroidlib.cookie.Cookie;







public interface CookieStore {

    






    void addCookie(Cookie cookie);

    




    List<Cookie> getCookies();

    





    boolean clearExpired(Date date);

    


    void clear();

}
