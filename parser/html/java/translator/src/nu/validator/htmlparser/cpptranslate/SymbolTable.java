




































package nu.validator.htmlparser.cpptranslate;

import java.util.HashMap;
import java.util.Map;

public class SymbolTable {
    
    public final Map<String, String> cppDefinesByJavaNames = new HashMap<String, String>();

    private final Map<StringPair, Type> fields = new HashMap<StringPair, Type>();
    
    private final Map<StringPair, Type> methodReturns = new HashMap<StringPair, Type>();
    
    






    public boolean isNotAnAttributeOrElementName(String name) {
        return !("ATTRIBUTE_HASHES".equals(name)
                || "ATTRIBUTE_NAMES".equals(name)
                || "ELEMENT_HASHES".equals(name)
                || "ELEMENT_NAMES".equals(name) || "ALL_NO_NS".equals(name));
    }
    
    public void putFieldType(String klazz, String field, Type type) {
        fields.put(new StringPair(klazz, field), type);
    }
    
    public void putMethodReturnType(String klazz, String method, Type type) {
        methodReturns.put(new StringPair(klazz, method), type);
    }
    
    public Type getFieldType(String klazz, String field) {
        return fields.get(new StringPair(klazz, field));
    }
    
    public Type getMethodReturnType(String klazz, String method) {
        return methodReturns.get(new StringPair(klazz, method));
    }
}
