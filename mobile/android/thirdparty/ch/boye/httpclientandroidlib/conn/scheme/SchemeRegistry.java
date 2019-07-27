

























package ch.boye.httpclientandroidlib.conn.scheme;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;









@ThreadSafe
@Deprecated
public final class SchemeRegistry {

    
    private final ConcurrentHashMap<String,Scheme> registeredSchemes;

    


    public SchemeRegistry() {
        super();
        registeredSchemes = new ConcurrentHashMap<String,Scheme>();
    }

    









    public final Scheme getScheme(final String name) {
        final Scheme found = get(name);
        if (found == null) {
            throw new IllegalStateException
                ("Scheme '"+name+"' not registered.");
        }
        return found;
    }

    










    public final Scheme getScheme(final HttpHost host) {
        Args.notNull(host, "Host");
        return getScheme(host.getSchemeName());
    }

    







    public final Scheme get(final String name) {
        Args.notNull(name, "Scheme name");
        
        
        final Scheme found = registeredSchemes.get(name);
        return found;
    }

    









    public final Scheme register(final Scheme sch) {
        Args.notNull(sch, "Scheme");
        final Scheme old = registeredSchemes.put(sch.getName(), sch);
        return old;
    }

    







    public final Scheme unregister(final String name) {
        Args.notNull(name, "Scheme name");
        
        
        final Scheme gone = registeredSchemes.remove(name);
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

