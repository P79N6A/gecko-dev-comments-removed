


























package ch.boye.httpclientandroidlib.cookie;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.config.Lookup;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;










@ThreadSafe
@Deprecated
public final class CookieSpecRegistry implements Lookup<CookieSpecProvider> {

    private final ConcurrentHashMap<String,CookieSpecFactory> registeredSpecs;

    public CookieSpecRegistry() {
        super();
        this.registeredSpecs = new ConcurrentHashMap<String,CookieSpecFactory>();
    }

    










    public void register(final String name, final CookieSpecFactory factory) {
         Args.notNull(name, "Name");
        Args.notNull(factory, "Cookie spec factory");
        registeredSpecs.put(name.toLowerCase(Locale.ENGLISH), factory);
    }

    




    public void unregister(final String id) {
         Args.notNull(id, "Id");
         registeredSpecs.remove(id.toLowerCase(Locale.ENGLISH));
    }

    










    public CookieSpec getCookieSpec(final String name, final HttpParams params)
        throws IllegalStateException {

        Args.notNull(name, "Name");
        final CookieSpecFactory factory = registeredSpecs.get(name.toLowerCase(Locale.ENGLISH));
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

    public CookieSpecProvider lookup(final String name) {
        return new CookieSpecProvider() {

            public CookieSpec create(final HttpContext context) {
                final HttpRequest request = (HttpRequest) context.getAttribute(
                        ExecutionContext.HTTP_REQUEST);
                return getCookieSpec(name, request.getParams());
            }

        };
    }

}
