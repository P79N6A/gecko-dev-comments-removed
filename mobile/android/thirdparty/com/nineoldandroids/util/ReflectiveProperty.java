














package com.nineoldandroids.util;

import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;





class ReflectiveProperty<T, V> extends Property<T, V> {

    private static final String PREFIX_GET = "get";
    private static final String PREFIX_IS = "is";
    private static final String PREFIX_SET = "set";
    private Method mSetter;
    private Method mGetter;
    private Field mField;

    









    public ReflectiveProperty(Class<T> propertyHolder, Class<V> valueType, String name) {
         
        super(valueType, name);
        char firstLetter = Character.toUpperCase(name.charAt(0));
        String theRest = name.substring(1);
        String capitalizedName = firstLetter + theRest;
        String getterName = PREFIX_GET + capitalizedName;
        try {
            mGetter = propertyHolder.getMethod(getterName, (Class<?>[]) null);
        } catch (NoSuchMethodException e) {
            try {
                


                mGetter = propertyHolder.getDeclaredMethod(getterName, (Class<?>[]) null);
                mGetter.setAccessible(true);
            } catch (NoSuchMethodException e2) {
                
                getterName = PREFIX_IS + capitalizedName;
                try {
                    mGetter = propertyHolder.getMethod(getterName, (Class<?>[]) null);
                } catch (NoSuchMethodException e3) {
                    try {
                        


                        mGetter = propertyHolder.getDeclaredMethod(getterName, (Class<?>[]) null);
                        mGetter.setAccessible(true);
                    } catch (NoSuchMethodException e4) {
                        
                        try {
                            mField = propertyHolder.getField(name);
                            Class fieldType = mField.getType();
                            if (!typesMatch(valueType, fieldType)) {
                                throw new NoSuchPropertyException("Underlying type (" + fieldType + ") " +
                                        "does not match Property type (" + valueType + ")");
                            }
                            return;
                        } catch (NoSuchFieldException e5) {
                            
                            throw new NoSuchPropertyException("No accessor method or field found for"
                                    + " property with name " + name);
                        }
                    }
                }
            }
        }
        Class getterType = mGetter.getReturnType();
        
        if (!typesMatch(valueType, getterType)) {
            throw new NoSuchPropertyException("Underlying type (" + getterType + ") " +
                    "does not match Property type (" + valueType + ")");
        }
        String setterName = PREFIX_SET + capitalizedName;
        try {
            
            
            mSetter = propertyHolder.getDeclaredMethod(setterName, getterType);
            mSetter.setAccessible(true);
        } catch (NoSuchMethodException ignored) {
            
        }
    }

    






    private boolean typesMatch(Class<V> valueType, Class getterType) {
        if (getterType != valueType) {
            if (getterType.isPrimitive()) {
                return (getterType == float.class && valueType == Float.class) ||
                        (getterType == int.class && valueType == Integer.class) ||
                        (getterType == boolean.class && valueType == Boolean.class) ||
                        (getterType == long.class && valueType == Long.class) ||
                        (getterType == double.class && valueType == Double.class) ||
                        (getterType == short.class && valueType == Short.class) ||
                        (getterType == byte.class && valueType == Byte.class) ||
                        (getterType == char.class && valueType == Character.class);
            }
            return false;
        }
        return true;
    }

    @Override
    public void set(T object, V value) {
        if (mSetter != null) {
            try {
                mSetter.invoke(object, value);
            } catch (IllegalAccessException e) {
                throw new AssertionError();
            } catch (InvocationTargetException e) {
                throw new RuntimeException(e.getCause());
            }
        } else if (mField != null) {
            try {
                mField.set(object, value);
            } catch (IllegalAccessException e) {
                throw new AssertionError();
            }
        } else {
            throw new UnsupportedOperationException("Property " + getName() +" is read-only");
        }
    }

    @Override
    public V get(T object) {
        if (mGetter != null) {
            try {
                return (V) mGetter.invoke(object, (Object[])null);
            } catch (IllegalAccessException e) {
                throw new AssertionError();
            } catch (InvocationTargetException e) {
                throw new RuntimeException(e.getCause());
            }
        } else if (mField != null) {
            try {
                return (V) mField.get(object);
            } catch (IllegalAccessException e) {
                throw new AssertionError();
            }
        }
        
        throw new AssertionError();
    }

    


    @Override
    public boolean isReadOnly() {
        return (mSetter == null && mField == null);
    }
}
