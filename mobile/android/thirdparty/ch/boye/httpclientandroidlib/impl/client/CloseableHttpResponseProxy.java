


























package ch.boye.httpclientandroidlib.impl.client;

import java.io.IOException;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.util.EntityUtils;




@NotThreadSafe
class CloseableHttpResponseProxy implements InvocationHandler {

    private final HttpResponse original;

    CloseableHttpResponseProxy(final HttpResponse original) {
        super();
        this.original = original;
    }

    public void close() throws IOException {
        final HttpEntity entity = this.original.getEntity();
        EntityUtils.consume(entity);
    }

    public Object invoke(
            final Object proxy, final Method method, final Object[] args) throws Throwable {
        final String mname = method.getName();
        if (mname.equals("close")) {
            close();
            return null;
        } else {
            try {
                return method.invoke(original, args);
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

    public static CloseableHttpResponse newProxy(final HttpResponse original) {
        return (CloseableHttpResponse) Proxy.newProxyInstance(
                CloseableHttpResponseProxy.class.getClassLoader(),
                new Class<?>[] { CloseableHttpResponse.class },
                new CloseableHttpResponseProxy(original));
    }

}
