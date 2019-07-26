



package org.mozilla.gecko.annotationProcessors.utils;

import java.lang.reflect.Method;
import java.util.Comparator;

public class AlphabeticMethodComparator implements Comparator<Method> {
    @Override
    public int compare(Method lhs, Method rhs) {
        
        String lName = lhs.getName();
        String rName = rhs.getName();

        int ret = lName.compareTo(rName);
        if (ret != 0) {
            return ret;
        }

        
        lName = Utils.getTypeSignatureString(lhs);
        rName = Utils.getTypeSignatureString(rhs);

        return lName.compareTo(rName);
    }
}
