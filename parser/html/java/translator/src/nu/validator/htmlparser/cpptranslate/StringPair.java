




































package nu.validator.htmlparser.cpptranslate;

public class StringPair {

    



    public StringPair(String first, String second) {
        this.first = first;
        this.second = second;
    }

    private final String first;

    private final String second;

    


    @Override public boolean equals(Object o) {
        if (o instanceof StringPair) {
            StringPair other = (StringPair) o;
            return first.equals(other.first) && second.equals(other.second);
        }
        return false;
    }

    


    @Override public int hashCode() {
        return first.hashCode() ^ second.hashCode();
    }

}
