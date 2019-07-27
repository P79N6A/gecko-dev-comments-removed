


























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.Closeable;
import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;







@NotThreadSafe
class ResponseProxyHandler implements InvocationHandler {

    private static final Method CLOSE_METHOD;

    static {
        try {
            CLOSE_METHOD = Closeable.class.getMethod("close");
        } catch (final NoSuchMethodException ex) {
            throw new Error(ex);
        }
    }

    private final HttpResponse original;

    ResponseProxyHandler(final HttpResponse original) {
        super();
        this.original = original;
    }

    public void close() throws IOException {
        IOUtils.consume(original.getEntity());
    }

    public Object invoke(
            final Object proxy, final Method method, final Object[] args) throws Throwable {
        if (method.equals(CLOSE_METHOD)) {
            close();
            return null;
        } else {
            try {
                return method.invoke(this.original, args);
            } catch (final InvocationTargetException ex) {
                final Throwable cause = ex.getCause();
                if (cause != null) {
                    throw cause;
                } else {
                    throw ex;
                }
            }
        }
    }

}
