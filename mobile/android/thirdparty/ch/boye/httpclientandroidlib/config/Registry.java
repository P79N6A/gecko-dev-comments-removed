


























package ch.boye.httpclientandroidlib.config;

import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;






@ThreadSafe
public final class Registry<I> implements Lookup<I> {

    private final Map<String, I> map;

    Registry(final Map<String, I> map) {
        super();
        this.map = new ConcurrentHashMap<String, I>(map);
    }

    public I lookup(final String key) {
        if (key == null) {
            return null;
        }
        return map.get(key.toLowerCase(Locale.US));
    }

    @Override
    public String toString() {
        return map.toString();
    }

}
