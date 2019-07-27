


























package ch.boye.httpclientandroidlib.params;

import java.util.Set;











@Deprecated
public abstract class AbstractHttpParams implements HttpParams, HttpParamsNames {

    


    protected AbstractHttpParams() {
        super();
    }

    public long getLongParameter(final String name, final long defaultValue) {
        final Object param = getParameter(name);
        if (param == null) {
            return defaultValue;
        }
        return ((Long) param).longValue();
    }

    public HttpParams setLongParameter(final String name, final long value) {
        setParameter(name, Long.valueOf(value));
        return this;
    }

    public int getIntParameter(final String name, final int defaultValue) {
        final Object param = getParameter(name);
        if (param == null) {
            return defaultValue;
        }
        return ((Integer) param).intValue();
    }

    public HttpParams setIntParameter(final String name, final int value) {
        setParameter(name, Integer.valueOf(value));
        return this;
    }

    public double getDoubleParameter(final String name, final double defaultValue) {
        final Object param = getParameter(name);
        if (param == null) {
            return defaultValue;
        }
        return ((Double) param).doubleValue();
    }

    public HttpParams setDoubleParameter(final String name, final double value) {
        setParameter(name, Double.valueOf(value));
        return this;
    }

    public boolean getBooleanParameter(final String name, final boolean defaultValue) {
        final Object param = getParameter(name);
        if (param == null) {
            return defaultValue;
        }
        return ((Boolean) param).booleanValue();
    }

    public HttpParams setBooleanParameter(final String name, final boolean value) {
        setParameter(name, value ? Boolean.TRUE : Boolean.FALSE);
        return this;
    }

    public boolean isParameterTrue(final String name) {
        return getBooleanParameter(name, false);
    }

    public boolean isParameterFalse(final String name) {
        return !getBooleanParameter(name, false);
    }

    







    public Set<String> getNames(){
        throw new UnsupportedOperationException();
    }

} 
