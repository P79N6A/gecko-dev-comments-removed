


























package ch.boye.httpclientandroidlib.params;

import java.io.Serializable;
import java.util.Map;
import java.util.HashMap;
import java.util.Iterator;

import ch.boye.httpclientandroidlib.params.HttpParams;









public class BasicHttpParams extends AbstractHttpParams implements Serializable, Cloneable {

    private static final long serialVersionUID = -7086398485908701455L;

    
    private final HashMap parameters = new HashMap();

    public BasicHttpParams() {
        super();
    }

    public Object getParameter(final String name) {
        return this.parameters.get(name);
    }

    public HttpParams setParameter(final String name, final Object value) {
        this.parameters.put(name, value);
        return this;
    }

    public boolean removeParameter(String name) {
        
        if (this.parameters.containsKey(name)) {
            this.parameters.remove(name);
            return true;
        } else {
            return false;
        }
    }

    





    public void setParameters(final String[] names, final Object value) {
        for (int i = 0; i < names.length; i++) {
            setParameter(names[i], value);
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
        } catch (CloneNotSupportedException ex) {
            throw new UnsupportedOperationException("Cloning not supported");
        }
    }

    



    public Object clone() throws CloneNotSupportedException {
        BasicHttpParams clone = (BasicHttpParams) super.clone();
        copyParams(clone);
        return clone;
    }

    protected void copyParams(HttpParams target) {
        Iterator iter = parameters.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry me = (Map.Entry) iter.next();
            if (me.getKey() instanceof String)
                target.setParameter((String)me.getKey(), me.getValue());
        }
    }

}
