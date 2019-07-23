




































package nu.validator.htmlparser.cpptranslate;

import java.util.HashMap;
import java.util.Map;

public class LocalSymbolTable {

    private final Map<String, Type> locals = new HashMap<String, Type>();

    private final String javaClassName;
    
    private final SymbolTable delegate;    

    



    public LocalSymbolTable(String javaClassName, SymbolTable delegate) {
        this.javaClassName = javaClassName;
        this.delegate = delegate;
    }

    public void putLocalType(String name, Type type) {
        locals.put(name, type);
    }

    





    public Type getVariableType(String klazz, String variable) {
        if (klazz == null) {
            Type type = locals.get(variable);
            if (type != null) {
                return type;
            }
        }
        return delegate.getFieldType(((klazz == null || "this".equals(klazz)) ? javaClassName : klazz), variable);
    }

    





    public Type getMethodReturnType(String klazz, String method) {
        return delegate.getMethodReturnType(((klazz == null || "this".equals(klazz)) ? javaClassName : klazz), method);
    }
}
