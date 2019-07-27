



package org.mozilla.gecko.mozglue.generatorannotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;














@Target({ElementType.FIELD, ElementType.METHOD, ElementType.CONSTRUCTOR})
@Retention(RetentionPolicy.RUNTIME)
public @interface WrapElementForJNI {
    
    
    String stubName() default "";

    






    boolean allowMultithread() default false;

    



    boolean noThrow() default false;

    boolean narrowChars() default false;
}
