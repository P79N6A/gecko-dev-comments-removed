


























package ch.boye.httpclientandroidlib.conn.routing;

import java.net.InetAddress;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.LangUtils;

import ch.boye.httpclientandroidlib.HttpHost;








@Immutable
public final class HttpRoute implements RouteInfo, Cloneable {

    private static final HttpHost[] EMPTY_HTTP_HOST_ARRAY = new HttpHost[]{};

    
    private final HttpHost targetHost;

    



    private final InetAddress localAddress;

    
    private final HttpHost[] proxyChain;

    
    private final TunnelType tunnelled;

    
    private final LayerType layered;

    
    private final boolean secure;


    



















    private HttpRoute(InetAddress local,
                      HttpHost target, HttpHost[] proxies,
                      boolean secure,
                      TunnelType tunnelled, LayerType layered) {
        if (target == null) {
            throw new IllegalArgumentException
                ("Target host may not be null.");
        }
        if (proxies == null) {
            throw new IllegalArgumentException
                ("Proxies may not be null.");
        }
        if ((tunnelled == TunnelType.TUNNELLED) && (proxies.length == 0)) {
            throw new IllegalArgumentException
                ("Proxy required if tunnelled.");
        }

        
        if (tunnelled == null)
            tunnelled = TunnelType.PLAIN;
        if (layered == null)
            layered = LayerType.PLAIN;

        this.targetHost   = target;
        this.localAddress = local;
        this.proxyChain   = proxies;
        this.secure       = secure;
        this.tunnelled    = tunnelled;
        this.layered      = layered;
    }


    












    public HttpRoute(HttpHost target, InetAddress local, HttpHost[] proxies,
                     boolean secure, TunnelType tunnelled, LayerType layered) {
        this(local, target, toChain(proxies), secure, tunnelled, layered);
    }


    
















    public HttpRoute(HttpHost target, InetAddress local, HttpHost proxy,
                     boolean secure, TunnelType tunnelled, LayerType layered) {
        this(local, target, toChain(proxy), secure, tunnelled, layered);
    }


    









    public HttpRoute(HttpHost target, InetAddress local, boolean secure) {
        this(local, target, EMPTY_HTTP_HOST_ARRAY, secure, TunnelType.PLAIN, LayerType.PLAIN);
    }


    




    public HttpRoute(HttpHost target) {
        this(null, target, EMPTY_HTTP_HOST_ARRAY, false, TunnelType.PLAIN, LayerType.PLAIN);
    }


    












    public HttpRoute(HttpHost target, InetAddress local, HttpHost proxy,
                     boolean secure) {
        this(local, target, toChain(proxy), secure,
             secure ? TunnelType.TUNNELLED : TunnelType.PLAIN,
             secure ? LayerType.LAYERED    : LayerType.PLAIN);
        if (proxy == null) {
            throw new IllegalArgumentException
                ("Proxy host may not be null.");
        }
    }


    






    private static HttpHost[] toChain(HttpHost proxy) {
        if (proxy == null)
            return EMPTY_HTTP_HOST_ARRAY;

        return new HttpHost[]{ proxy };
    }


    







    private static HttpHost[] toChain(HttpHost[] proxies) {
        if ((proxies == null) || (proxies.length < 1))
            return EMPTY_HTTP_HOST_ARRAY;

        for (HttpHost proxy : proxies) {
            if (proxy == null)
                throw new IllegalArgumentException
                        ("Proxy chain may not contain null elements.");
        }

        
        HttpHost[] result = new HttpHost[proxies.length];
        System.arraycopy(proxies, 0, result, 0, proxies.length);

        return result;
    }



    
    public final HttpHost getTargetHost() {
        return this.targetHost;
    }


    
    public final InetAddress getLocalAddress() {
        return this.localAddress;
    }


    public final int getHopCount() {
        return proxyChain.length+1;
    }


    public final HttpHost getHopTarget(int hop) {
        if (hop < 0)
            throw new IllegalArgumentException
                ("Hop index must not be negative: " + hop);
        final int hopcount = getHopCount();
        if (hop >= hopcount)
            throw new IllegalArgumentException
                ("Hop index " + hop +
                 " exceeds route length " + hopcount);

        HttpHost result = null;
        if (hop < hopcount-1)
            result = this.proxyChain[hop];
        else
            result = this.targetHost;

        return result;
    }


    public final HttpHost getProxyHost() {
        return (this.proxyChain.length == 0) ? null : this.proxyChain[0];
    }


    public final TunnelType getTunnelType() {
        return this.tunnelled;
    }


    public final boolean isTunnelled() {
        return (this.tunnelled == TunnelType.TUNNELLED);
    }


    public final LayerType getLayerType() {
        return this.layered;
    }


    public final boolean isLayered() {
        return (this.layered == LayerType.LAYERED);
    }


    public final boolean isSecure() {
        return this.secure;
    }


    







    @Override
    public final boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj instanceof HttpRoute) {
            HttpRoute that = (HttpRoute) obj;
            return 
                
                (this.secure    == that.secure) &&
                (this.tunnelled == that.tunnelled) &&
                (this.layered   == that.layered) &&
                LangUtils.equals(this.targetHost, that.targetHost) &&
                LangUtils.equals(this.localAddress, that.localAddress) &&
                LangUtils.equals(this.proxyChain, that.proxyChain);
        } else {
            return false;
        }
    }


    




    @Override
    public final int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.targetHost);
        hash = LangUtils.hashCode(hash, this.localAddress);
        for (int i = 0; i < this.proxyChain.length; i++) {
            hash = LangUtils.hashCode(hash, this.proxyChain[i]);
        }
        hash = LangUtils.hashCode(hash, this.secure);
        hash = LangUtils.hashCode(hash, this.tunnelled);
        hash = LangUtils.hashCode(hash, this.layered);
        return hash;
    }


    




    @Override
    public final String toString() {
        StringBuilder cab = new StringBuilder(50 + getHopCount()*30);

        cab.append("HttpRoute[");
        if (this.localAddress != null) {
            cab.append(this.localAddress);
            cab.append("->");
        }
        cab.append('{');
        if (this.tunnelled == TunnelType.TUNNELLED)
            cab.append('t');
        if (this.layered == LayerType.LAYERED)
            cab.append('l');
        if (this.secure)
            cab.append('s');
        cab.append("}->");
        for (HttpHost aProxyChain : this.proxyChain) {
            cab.append(aProxyChain);
            cab.append("->");
        }
        cab.append(this.targetHost);
        cab.append(']');

        return cab.toString();
    }


    
    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
