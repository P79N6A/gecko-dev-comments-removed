


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.LangUtils;






@NotThreadSafe
public class BasicHeaderElement implements HeaderElement, Cloneable {

    private final String name;
    private final String value;
    private final NameValuePair[] parameters;

    







    public BasicHeaderElement(
            final String name,
            final String value,
            final NameValuePair[] parameters) {
        super();
        this.name = Args.notNull(name, "Name");
        this.value = value;
        if (parameters != null) {
            this.parameters = parameters;
        } else {
            this.parameters = new NameValuePair[] {};
        }
    }

    





    public BasicHeaderElement(final String name, final String value) {
       this(name, value, null);
    }

    public String getName() {
        return this.name;
    }

    public String getValue() {
        return this.value;
    }

    public NameValuePair[] getParameters() {
        return this.parameters.clone();
    }

    public int getParameterCount() {
        return this.parameters.length;
    }

    public NameValuePair getParameter(final int index) {
        
        return this.parameters[index];
    }

    public NameValuePair getParameterByName(final String name) {
        Args.notNull(name, "Name");
        NameValuePair found = null;
        for (final NameValuePair current : this.parameters) {
            if (current.getName().equalsIgnoreCase(name)) {
                found = current;
                break;
            }
        }
        return found;
    }

    @Override
    public boolean equals(final Object object) {
        if (this == object) {
            return true;
        }
        if (object instanceof HeaderElement) {
            final BasicHeaderElement that = (BasicHeaderElement) object;
            return this.name.equals(that.name)
                && LangUtils.equals(this.value, that.value)
                && LangUtils.equals(this.parameters, that.parameters);
        } else {
            return false;
        }
    }

    @Override
    public int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.name);
        hash = LangUtils.hashCode(hash, this.value);
        for (final NameValuePair parameter : this.parameters) {
            hash = LangUtils.hashCode(hash, parameter);
        }
        return hash;
    }

    @Override
    public String toString() {
        final StringBuilder buffer = new StringBuilder();
        buffer.append(this.name);
        if (this.value != null) {
            buffer.append("=");
            buffer.append(this.value);
        }
        for (final NameValuePair parameter : this.parameters) {
            buffer.append("; ");
            buffer.append(parameter);
        }
        return buffer.toString();
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        
        
        return super.clone();
    }

}

