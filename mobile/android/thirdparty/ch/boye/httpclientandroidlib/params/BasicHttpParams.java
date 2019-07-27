


























package ch.boye.httpclientandroidlib.params;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;












@Deprecated
@ThreadSafe
public class BasicHttpParams extends AbstractHttpParams implements Serializable, Cloneable {

    private static final long serialVersionUID = -7086398485908701455L;

    
    private final Map<String, Object> parameters = new ConcurrentHashMap<String, Object>();

    public BasicHttpParams() {
        super();
    }

    public Object getParameter(final String name) {
        return this.parameters.get(name);
    }

    public HttpParams setParameter(final String name, final Object value) {
        if (name == null) {
            return this;
        }
        if (value != null) {
            this.parameters.put(name, value);
        } else {
            this.parameters.remove(name);
        }
        return this;
    }

    public boolean removeParameter(final String name) {
        
        if (this.parameters.containsKey(name)) {
            this.parameters.remove(name);
            return true;
        } else {
            return false;
        }
    }

    





    public void setParameters(final String[] names, final Object value) {
        for (final String name : names) {
            setParameter(name, value);
        }
    }

    










    public boolean isParameterSet(final String name) {
        return getParameter(name) != null;
    }

    









    public boolean isParameterSetLocally(final String name) {
        return this.parameters.get(name) != null;
    }

    


    public void clear() {
        this.parameters.clear();
    }

    








    public HttpParams copy() {
        try {
            return (HttpParams) clone();
        } catch (final CloneNotSupportedException ex) {
            throw new UnsupportedOperationException("Cloning not supported");
        }
    }

    



    @Override
    public Object clone() throws CloneNotSupportedException {
        final BasicHttpParams clone = (BasicHttpParams) super.clone();
        copyParams(clone);
        return clone;
    }

    






    public void copyParams(final HttpParams target) {
        for (final Map.Entry<String, Object> me : this.parameters.entrySet()) {
            target.setParameter(me.getKey(), me.getValue());
        }
    }

    








    @Override
    public Set<String> getNames() {
        return new HashSet<String>(this.parameters.keySet());
    }
}
