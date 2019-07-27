



package org.mozilla.gecko.annotationProcessors.utils;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.util.Comparator;

public class AlphabeticAnnotatableEntityComparator<T extends Member> implements Comparator<T> {
    @Override
    public int compare(T aLhs, T aRhs) {
        
        boolean lIsConstructor = aLhs instanceof Constructor;
        boolean rIsConstructor = aRhs instanceof Constructor;
        boolean lIsMethod = aLhs instanceof Method;
        boolean rIsField = aRhs instanceof Field;

        if (lIsConstructor) {
            if (!rIsConstructor) {
                return -1;
            }
        } else if (lIsMethod) {
            if (rIsConstructor) {
                return 1;
            } else if (rIsField) {
                return -1;
            }
        } else {
            if (!rIsField) {
                return 1;
            }
        }

        
        if (aLhs instanceof Method) {
            return compare((Method) aLhs, (Method) aRhs);
        } else if (aLhs instanceof Field) {
            return compare((Field) aLhs, (Field) aRhs);
        } else {
            return compare((Constructor) aLhs, (Constructor) aRhs);
        }
    }

    
    private static int compare(Method aLhs, Method aRhs) {
        
        String lName = aLhs.getName();
        String rName = aRhs.getName();

        int ret = lName.compareTo(rName);
        if (ret != 0) {
            return ret;
        }

        
        lName = Utils.getTypeSignatureStringForMethod(aLhs);
        rName = Utils.getTypeSignatureStringForMethod(aRhs);

        return lName.compareTo(rName);
    }

    private static int compare(Constructor<?> aLhs, Constructor<?> aRhs) {
        
        String lName = Utils.getTypeSignatureString(aLhs);
        String rName = Utils.getTypeSignatureString(aRhs);

        return lName.compareTo(rName);
    }

    private static int compare(Field aLhs, Field aRhs) {
        
        String lName = aLhs.getName();
        String rName = aRhs.getName();

        return lName.compareTo(rName);
    }
}
