



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fFail;

import java.lang.reflect.Field;

import android.content.Context;
import android.view.View;









public final class FrameworkHelper {

    private FrameworkHelper() {  }

    private static Field getClassField(final Class<?> clazz, final String fieldName)
            throws NoSuchFieldException {
        Class<?> cls = clazz;
        do {
            try {
                return cls.getDeclaredField(fieldName);
            } catch (final Exception e) {
                
                
                
                
                
                
                cls = cls.getSuperclass();
            }
        } while (cls != null);
        
        
        
        
        return clazz.getField(fieldName);
    }

    private static Object getField(final Object obj, final String fieldName) {
        try {
            final Field field = getClassField(obj.getClass(), fieldName);
            final boolean accessible = field.isAccessible();
            field.setAccessible(true);
            final Object ret = field.get(obj);
            field.setAccessible(accessible);
            return ret;
        } catch (final NoSuchFieldException e) {
            
            
            fFail("Argument field should be a valid field name: " + e.toString());
        } catch (final IllegalAccessException e) {
            
            fFail("Field should be accessible: " + e.toString());
        }
        throw new IllegalStateException("Should not continue from previous failures");
    }

    private static void setField(final Object obj, final String fieldName, final Object value) {
        try {
            final Field field = getClassField(obj.getClass(), fieldName);
            final boolean accessible = field.isAccessible();
            field.setAccessible(true);
            field.set(obj, value);
            field.setAccessible(accessible);
            return;
        } catch (final NoSuchFieldException e) {
            
            
            fFail("Argument field should be a valid field name: " + e.toString());
        } catch (final IllegalAccessException e) {
            
            fFail("Field should be accessible: " + e.toString());
        }
        throw new IllegalStateException("Cannot continue from previous failures");
    }

    public static Context getViewContext(final View v) {
        return (Context) getField(v, "mContext");
    }

    public static void setViewContext(final View v, final Context c) {
        setField(v, "mContext", c);
    }
}
