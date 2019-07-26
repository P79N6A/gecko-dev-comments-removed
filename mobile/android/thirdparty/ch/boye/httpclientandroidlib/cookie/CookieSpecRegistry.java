


























package ch.boye.httpclientandroidlib.cookie;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import ch.boye.httpclientandroidlib.params.HttpParams;









@ThreadSafe
public final class CookieSpecRegistry {

    private final ConcurrentHashMap<String,CookieSpecFactory> registeredSpecs;

    public CookieSpecRegistry() {
        super();
        this.registeredSpecs = new ConcurrentHashMap<String,CookieSpecFactory>();
    }

    










    public void register(final String name, final CookieSpecFactory factory) {
         if (name == null) {
             throw new IllegalArgumentException("Name may not be null");
         }
        if (factory == null) {
            throw new IllegalArgumentException("Cookie spec factory may not be null");
        }
        registeredSpecs.put(name.toLowerCase(Locale.ENGLISH), factory);
    }

    




    public void unregister(final String id) {
         if (id == null) {
             throw new IllegalArgumentException("Id may not be null");
         }
         registeredSpecs.remove(id.toLowerCase(Locale.ENGLISH));
    }

    










    public CookieSpec getCookieSpec(final String name, final HttpParams params)
        throws IllegalStateException {

        if (name == null) {
            throw new IllegalArgumentException("Name may not be null");
        }
        CookieSpecFactory factory = registeredSpecs.get(name.toLowerCase(Locale.ENGLISH));
        if (factory != null) {
            return factory.newInstance(params);
        } else {
            throw new IllegalStateException("Unsupported cookie spec: " + name);
        }
    }

    








    public CookieSpec getCookieSpec(final String name)
        throws IllegalStateException {
        return getCookieSpec(name, null);
    }

    








    public List<String> getSpecNames(){
        return new ArrayList<String>(registeredSpecs.keySet());
    }

    





    public void setItems(final Map<String, CookieSpecFactory> map) {
        if (map == null) {
            return;
        }
        registeredSpecs.clear();
        registeredSpecs.putAll(map);
    }

}
