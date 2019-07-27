


























package ch.boye.httpclientandroidlib.protocol;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;









@ThreadSafe
public class BasicHttpContext implements HttpContext {

    private final HttpContext parentContext;
    private final Map<String, Object> map;

    public BasicHttpContext() {
        this(null);
    }

    public BasicHttpContext(final HttpContext parentContext) {
        super();
        this.map = new ConcurrentHashMap<String, Object>();
        this.parentContext = parentContext;
    }

    public Object getAttribute(final String id) {
        Args.notNull(id, "Id");
        Object obj = this.map.get(id);
        if (obj == null && this.parentContext != null) {
            obj = this.parentContext.getAttribute(id);
        }
        return obj;
    }

    public void setAttribute(final String id, final Object obj) {
        Args.notNull(id, "Id");
        if (obj != null) {
            this.map.put(id, obj);
        } else {
            this.map.remove(id);
        }
    }

    public Object removeAttribute(final String id) {
        Args.notNull(id, "Id");
        return this.map.remove(id);
    }

    


    public void clear() {
        this.map.clear();
    }

    @Override
    public String toString() {
        return this.map.toString();
    }

}
