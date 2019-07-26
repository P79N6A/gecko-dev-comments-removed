


























package ch.boye.httpclientandroidlib;

import java.io.Serializable;
import java.util.Locale;

import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
import ch.boye.httpclientandroidlib.util.LangUtils;









public final class HttpHost implements Cloneable, Serializable {

    private static final long serialVersionUID = -7529410654042457626L;

    
    public static final String DEFAULT_SCHEME_NAME = "http";

    
    protected final String hostname;

    
    protected final String lcHostname;


    
    protected final int port;

    
    protected final String schemeName;


    










    public HttpHost(final String hostname, int port, final String scheme) {
        super();
        if (hostname == null) {
            throw new IllegalArgumentException("Host name may not be null");
        }
        this.hostname   = hostname;
        this.lcHostname = hostname.toLowerCase(Locale.ENGLISH);
        if (scheme != null) {
            this.schemeName = scheme.toLowerCase(Locale.ENGLISH);
        } else {
            this.schemeName = DEFAULT_SCHEME_NAME;
        }
        this.port = port;
    }

    






    public HttpHost(final String hostname, int port) {
        this(hostname, port, null);
    }

    




    public HttpHost(final String hostname) {
        this(hostname, -1, null);
    }

    




    public HttpHost (final HttpHost httphost) {
        this(httphost.hostname, httphost.port, httphost.schemeName);
    }

    




    public String getHostName() {
        return this.hostname;
    }

    




    public int getPort() {
        return this.port;
    }

    




    public String getSchemeName() {
        return this.schemeName;
    }

    




    public String toURI() {
        CharArrayBuffer buffer = new CharArrayBuffer(32);
        buffer.append(this.schemeName);
        buffer.append("://");
        buffer.append(this.hostname);
        if (this.port != -1) {
            buffer.append(':');
            buffer.append(Integer.toString(this.port));
        }
        return buffer.toString();
    }


    




    public String toHostString() {
        if (this.port != -1) {
            
            CharArrayBuffer buffer = new CharArrayBuffer(this.hostname.length() + 6);
            buffer.append(this.hostname);
            buffer.append(":");
            buffer.append(Integer.toString(this.port));
            return buffer.toString();
        } else {
            return this.hostname;
        }
    }


    public String toString() {
        return toURI();
    }


    public boolean equals(final Object obj) {
        if (this == obj) return true;
        if (obj instanceof HttpHost) {
            HttpHost that = (HttpHost) obj;
            return this.lcHostname.equals(that.lcHostname)
                && this.port == that.port
                && this.schemeName.equals(that.schemeName);
        } else {
            return false;
        }
    }

    


    public int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.lcHostname);
        hash = LangUtils.hashCode(hash, this.port);
        hash = LangUtils.hashCode(hash, this.schemeName);
        return hash;
    }

    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
