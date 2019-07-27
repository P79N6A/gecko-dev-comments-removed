

























package ch.boye.httpclientandroidlib.impl.client;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.CookieStore;
import ch.boye.httpclientandroidlib.cookie.Cookie;
import ch.boye.httpclientandroidlib.cookie.CookieIdentityComparator;







@ThreadSafe
public class BasicCookieStore implements CookieStore, Serializable {

    private static final long serialVersionUID = -7581093305228232025L;

    @GuardedBy("this")
    private final TreeSet<Cookie> cookies;

    public BasicCookieStore() {
        super();
        this.cookies = new TreeSet<Cookie>(new CookieIdentityComparator());
    }

    









    public synchronized void addCookie(final Cookie cookie) {
        if (cookie != null) {
            
            cookies.remove(cookie);
            if (!cookie.isExpired(new Date())) {
                cookies.add(cookie);
            }
        }
    }

    









    public synchronized void addCookies(final Cookie[] cookies) {
        if (cookies != null) {
            for (final Cookie cooky : cookies) {
                this.addCookie(cooky);
            }
        }
    }

    





    public synchronized List<Cookie> getCookies() {
        
        return new ArrayList<Cookie>(cookies);
    }

    







    public synchronized boolean clearExpired(final Date date) {
        if (date == null) {
            return false;
        }
        boolean removed = false;
        for (final Iterator<Cookie> it = cookies.iterator(); it.hasNext();) {
            if (it.next().isExpired(date)) {
                it.remove();
                removed = true;
            }
        }
        return removed;
    }

    


    public synchronized void clear() {
        cookies.clear();
    }

    @Override
    public synchronized String toString() {
        return cookies.toString();
    }

}
