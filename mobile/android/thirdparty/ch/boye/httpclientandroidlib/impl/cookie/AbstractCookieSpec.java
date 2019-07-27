


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.cookie.CookieAttributeHandler;
import ch.boye.httpclientandroidlib.cookie.CookieSpec;
import ch.boye.httpclientandroidlib.util.Args;









@NotThreadSafe 
public abstract class AbstractCookieSpec implements CookieSpec {

    


    private final Map<String, CookieAttributeHandler> attribHandlerMap;

    


    public AbstractCookieSpec() {
        super();
        this.attribHandlerMap = new HashMap<String, CookieAttributeHandler>(10);
    }

    public void registerAttribHandler(
            final String name, final CookieAttributeHandler handler) {
        Args.notNull(name, "Attribute name");
        Args.notNull(handler, "Attribute handler");
        this.attribHandlerMap.put(name, handler);
    }

    







    protected CookieAttributeHandler findAttribHandler(final String name) {
        return this.attribHandlerMap.get(name);
    }

    







    protected CookieAttributeHandler getAttribHandler(final String name) {
        final CookieAttributeHandler handler = findAttribHandler(name);
        if (handler == null) {
            throw new IllegalStateException("Handler not registered for " +
                                            name + " attribute.");
        } else {
            return handler;
        }
    }

    protected Collection<CookieAttributeHandler> getAttribHandlers() {
        return this.attribHandlerMap.values();
    }

}
