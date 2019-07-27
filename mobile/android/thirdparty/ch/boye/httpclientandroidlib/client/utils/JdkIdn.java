

























package ch.boye.httpclientandroidlib.client.utils;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class JdkIdn implements Idn {
    private final Method toUnicode;

    



    public JdkIdn() throws ClassNotFoundException {
        final Class<?> clazz = Class.forName("java.net.IDN");
        try {
            toUnicode = clazz.getMethod("toUnicode", String.class);
        } catch (final SecurityException e) {
            
            throw new IllegalStateException(e.getMessage(), e);
        } catch (final NoSuchMethodException e) {
            
            throw new IllegalStateException(e.getMessage(), e);
        }
    }

    public String toUnicode(final String punycode) {
        try {
            return (String) toUnicode.invoke(null, punycode);
        } catch (final IllegalAccessException e) {
            throw new IllegalStateException(e.getMessage(), e);
        } catch (final InvocationTargetException e) {
            final Throwable t = e.getCause();
            throw new RuntimeException(t.getMessage(), t);
        }
    }

}
