


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class BasicHeader implements Header, Cloneable, Serializable {

    private static final long serialVersionUID = -5427236326487562174L;

    private final String name;
    private final String value;

    





    public BasicHeader(final String name, final String value) {
        super();
        this.name = Args.notNull(name, "Name");
        this.value = value;
    }

    public String getName() {
        return this.name;
    }

    public String getValue() {
        return this.value;
    }

    @Override
    public String toString() {
        
        return BasicLineFormatter.INSTANCE.formatHeader(null, this).toString();
    }

    public HeaderElement[] getElements() throws ParseException {
        if (this.value != null) {
            
            return BasicHeaderValueParser.parseElements(this.value, null);
        } else {
            return new HeaderElement[] {};
        }
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
