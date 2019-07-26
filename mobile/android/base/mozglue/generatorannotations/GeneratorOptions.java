



package org.mozilla.gecko.mozglue.generatorannotations;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.RUNTIME)
public @interface GeneratorOptions {
    
    String generatedClassName() default "";
}