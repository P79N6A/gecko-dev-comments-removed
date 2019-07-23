




































package nu.validator.htmlparser.cpptranslate;

public class Type {

    





    public Type(String type, int arrayCount, boolean noLength, int modifiers) {
        this.type = type;
        this.arrayCount = arrayCount;
        this.noLength = noLength;
        this.modifiers = modifiers;
    }

    private final String type;
    
    private final int arrayCount;
    
    private final boolean noLength;
    
    private final int modifiers;

    




    public String getType() {
        return type;
    }

    




    public int getArrayCount() {
        return arrayCount;
    }

    




    public boolean isNoLength() {
        return noLength;
    }

    




    public int getModifiers() {
        return modifiers;
    }
    
}
