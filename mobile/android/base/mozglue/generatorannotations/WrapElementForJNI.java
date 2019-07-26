



package org.mozilla.gecko.mozglue.generatorannotations;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;














@Retention(RetentionPolicy.RUNTIME)
public @interface WrapElementForJNI {
    
    
    String stubName() default "";

    
    
    boolean generateStatic() default false;

    






    boolean allowMultithread() default false;
}
