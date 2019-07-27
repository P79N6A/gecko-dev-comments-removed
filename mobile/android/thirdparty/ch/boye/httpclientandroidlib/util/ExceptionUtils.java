

























package ch.boye.httpclientandroidlib.util;

import java.lang.reflect.Method;









@Deprecated
public final class ExceptionUtils {

    
    static private final Method INIT_CAUSE_METHOD = getInitCauseMethod();

    








    static private Method getInitCauseMethod() {
        try {
            final Class<?>[] paramsClasses = new Class[] { Throwable.class };
            return Throwable.class.getMethod("initCause", paramsClasses);
        } catch (final NoSuchMethodException e) {
            return null;
        }
    }

    





    public static void initCause(final Throwable throwable, final Throwable cause) {
        if (INIT_CAUSE_METHOD != null) {
            try {
                INIT_CAUSE_METHOD.invoke(throwable, cause);
            } catch (final Exception e) {
                
            }
        }
    }

    private ExceptionUtils() {
    }

}
