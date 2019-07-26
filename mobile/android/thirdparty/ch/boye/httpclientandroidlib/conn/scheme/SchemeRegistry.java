

























package ch.boye.httpclientandroidlib.conn.scheme;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;

import ch.boye.httpclientandroidlib.HttpHost;







@ThreadSafe
public final class SchemeRegistry {

    
    private final ConcurrentHashMap<String,Scheme> registeredSchemes;

    


    public SchemeRegistry() {
        super();
        registeredSchemes = new ConcurrentHashMap<String,Scheme>();
    }

    









    public final Scheme getScheme(String name) {
        Scheme found = get(name);
        if (found == null) {
            throw new IllegalStateException
                ("Scheme '"+name+"' not registered.");
        }
        return found;
    }

    










    public final Scheme getScheme(HttpHost host) {
        if (host == null) {
            throw new IllegalArgumentException("Host must not be null.");
        }
        return getScheme(host.getSchemeName());
    }

    







    public final Scheme get(String name) {
        if (name == null)
            throw new IllegalArgumentException("Name must not be null.");

        
        
        Scheme found = registeredSchemes.get(name);
        return found;
    }

    









    public final Scheme register(Scheme sch) {
        if (sch == null)
            throw new IllegalArgumentException("Scheme must not be null.");

        Scheme old = registeredSchemes.put(sch.getName(), sch);
        return old;
    }

    







    public final Scheme unregister(String name) {
        if (name == null)
            throw new IllegalArgumentException("Name must not be null.");

        
        
        Scheme gone = registeredSchemes.remove(name);
        return gone;
    }

    




    public final List<String> getSchemeNames() {
        return new ArrayList<String>(registeredSchemes.keySet());
    }

    





    public void setItems(final Map<String, Scheme> map) {
        if (map == null) {
            return;
        }
        registeredSchemes.clear();
        registeredSchemes.putAll(map);
    }

}

