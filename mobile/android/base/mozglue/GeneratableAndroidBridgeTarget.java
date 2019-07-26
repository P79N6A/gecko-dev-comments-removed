



package org.mozilla.gecko.mozglue;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;














@Retention(RetentionPolicy.RUNTIME)
public @interface GeneratableAndroidBridgeTarget {
    
    
    String stubName() default "";

    
    
    boolean generateStatic() default false;

    






    boolean allowMultithread() default false;
}
