

























package ch.boye.httpclientandroidlib.client.utils;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class CloneUtils {

    


    public static <T> T cloneObject(final T obj) throws CloneNotSupportedException {
        if (obj == null) {
            return null;
        }
        if (obj instanceof Cloneable) {
            final Class<?> clazz = obj.getClass ();
            final Method m;
            try {
                m = clazz.getMethod("clone", (Class[]) null);
            } catch (final NoSuchMethodException ex) {
                throw new NoSuchMethodError(ex.getMessage());
            }
            try {
                @SuppressWarnings("unchecked") 
                final T result = (T) m.invoke(obj, (Object []) null);
                return result;
            } catch (final InvocationTargetException ex) {
                final Throwable cause = ex.getCause();
                if (cause instanceof CloneNotSupportedException) {
                    throw ((CloneNotSupportedException) cause);
                } else {
                    throw new Error("Unexpected exception", cause);
                }
            } catch (final IllegalAccessException ex) {
                throw new IllegalAccessError(ex.getMessage());
            }
        } else {
            throw new CloneNotSupportedException();
        }
    }

    public static Object clone(final Object obj) throws CloneNotSupportedException {
        return cloneObject(obj);
    }

    


    private CloneUtils() {
    }

}
