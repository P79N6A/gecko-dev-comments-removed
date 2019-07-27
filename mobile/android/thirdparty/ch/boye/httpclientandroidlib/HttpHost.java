


























package ch.boye.httpclientandroidlib;

import java.io.Serializable;
import java.net.InetAddress;
import java.util.Locale;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.LangUtils;








@Immutable
public final class HttpHost implements Cloneable, Serializable {

    private static final long serialVersionUID = -7529410654042457626L;

    
    public static final String DEFAULT_SCHEME_NAME = "http";

    
    protected final String hostname;

    
    protected final String lcHostname;


    
    protected final int port;

    
    protected final String schemeName;

    protected final InetAddress address;

    










    public HttpHost(final String hostname, final int port, final String scheme) {
        super();
        this.hostname   = Args.notBlank(hostname, "Host name");
        this.lcHostname = hostname.toLowerCase(Locale.ENGLISH);
        if (scheme != null) {
            this.schemeName = scheme.toLowerCase(Locale.ENGLISH);
        } else {
            this.schemeName = DEFAULT_SCHEME_NAME;
        }
        this.port = port;
        this.address = null;
    }

    






    public HttpHost(final String hostname, final int port) {
        this(hostname, port, null);
    }

    




    public HttpHost(final String hostname) {
        this(hostname, -1, null);
    }

    












    public HttpHost(final InetAddress address, final int port, final String scheme) {
        super();
        this.address = Args.notNull(address, "Inet address");
        this.hostname = address.getHostAddress();
        this.lcHostname = this.hostname.toLowerCase(Locale.ENGLISH);
        if (scheme != null) {
            this.schemeName = scheme.toLowerCase(Locale.ENGLISH);
        } else {
            this.schemeName = DEFAULT_SCHEME_NAME;
        }
        this.port = port;
    }

    








    public HttpHost(final InetAddress address, final int port) {
        this(address, port, null);
    }

    






    public HttpHost(final InetAddress address) {
        this(address, -1, null);
    }

    




    public HttpHost (final HttpHost httphost) {
        super();
        Args.notNull(httphost, "HTTP host");
        this.hostname   = httphost.hostname;
        this.lcHostname = httphost.lcHostname;
        this.schemeName = httphost.schemeName;
        this.port = httphost.port;
        this.address = httphost.address;
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

    






    public InetAddress getAddress() {
        return this.address;
    }

    




    public String toURI() {
        final StringBuilder buffer = new StringBuilder();
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
            
            final StringBuilder buffer = new StringBuilder(this.hostname.length() + 6);
            buffer.append(this.hostname);
            buffer.append(":");
            buffer.append(Integer.toString(this.port));
            return buffer.toString();
        } else {
            return this.hostname;
        }
    }


    @Override
    public String toString() {
        return toURI();
    }


    @Override
    public boolean equals(final Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof HttpHost) {
            final HttpHost that = (HttpHost) obj;
            return this.lcHostname.equals(that.lcHostname)
                && this.port == that.port
                && this.schemeName.equals(that.schemeName);
        } else {
            return false;
        }
    }

    


    @Override
    public int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.lcHostname);
        hash = LangUtils.hashCode(hash, this.port);
        hash = LangUtils.hashCode(hash, this.schemeName);
        return hash;
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
