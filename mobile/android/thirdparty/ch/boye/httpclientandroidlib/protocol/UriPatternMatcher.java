


























package ch.boye.httpclientandroidlib.protocol;

import java.util.HashMap;
import java.util.Map;

import ch.boye.httpclientandroidlib.annotation.GuardedBy;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
















@ThreadSafe
public class UriPatternMatcher<T> {

    @GuardedBy("this")
    private final Map<String, T> map;

    public UriPatternMatcher() {
        super();
        this.map = new HashMap<String, T>();
    }

    





    public synchronized void register(final String pattern, final T obj) {
        Args.notNull(pattern, "URI request pattern");
        this.map.put(pattern, obj);
    }

    




    public synchronized void unregister(final String pattern) {
        if (pattern == null) {
            return;
        }
        this.map.remove(pattern);
    }

    


    @Deprecated
    public synchronized void setHandlers(final Map<String, T> map) {
        Args.notNull(map, "Map of handlers");
        this.map.clear();
        this.map.putAll(map);
    }

    


    @Deprecated
    public synchronized void setObjects(final Map<String, T> map) {
        Args.notNull(map, "Map of handlers");
        this.map.clear();
        this.map.putAll(map);
    }

    


    @Deprecated
    public synchronized Map<String, T> getObjects() {
        return this.map;
    }

    





    public synchronized T lookup(final String path) {
        Args.notNull(path, "Request path");
        
        T obj = this.map.get(path);
        if (obj == null) {
            
            String bestMatch = null;
            for (final String pattern : this.map.keySet()) {
                if (matchUriRequestPattern(pattern, path)) {
                    
                    if (bestMatch == null
                            || (bestMatch.length() < pattern.length())
                            || (bestMatch.length() == pattern.length() && pattern.endsWith("*"))) {
                        obj = this.map.get(pattern);
                        bestMatch = pattern;
                    }
                }
            }
        }
        return obj;
    }

    







    protected boolean matchUriRequestPattern(final String pattern, final String path) {
        if (pattern.equals("*")) {
            return true;
        } else {
            return
            (pattern.endsWith("*") && path.startsWith(pattern.substring(0, pattern.length() - 1))) ||
            (pattern.startsWith("*") && path.endsWith(pattern.substring(1, pattern.length())));
        }
    }

    @Override
    public String toString() {
        return this.map.toString();
    }

}
