


























package ch.boye.httpclientandroidlib.conn.routing;

import java.net.InetAddress;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.LangUtils;

import ch.boye.httpclientandroidlib.HttpHost;






@NotThreadSafe
public final class RouteTracker implements RouteInfo, Cloneable {

    
    private final HttpHost targetHost;

    



    private final InetAddress localAddress;

    
    

    
    private boolean connected;

    
    private HttpHost[] proxyChain;

    
    private TunnelType tunnelled;

    
    private LayerType layered;

    
    private boolean secure;

    







    public RouteTracker(HttpHost target, InetAddress local) {
        if (target == null) {
            throw new IllegalArgumentException("Target host may not be null.");
        }
        this.targetHost   = target;
        this.localAddress = local;
        this.tunnelled    = TunnelType.PLAIN;
        this.layered      = LayerType.PLAIN;
    }


    






    public RouteTracker(HttpRoute route) {
        this(route.getTargetHost(), route.getLocalAddress());
    }

    





    public final void connectTarget(boolean secure) {
        if (this.connected) {
            throw new IllegalStateException("Already connected.");
        }
        this.connected = true;
        this.secure = secure;
    }

    






    public final void connectProxy(HttpHost proxy, boolean secure) {
        if (proxy == null) {
            throw new IllegalArgumentException("Proxy host may not be null.");
        }
        if (this.connected) {
            throw new IllegalStateException("Already connected.");
        }
        this.connected  = true;
        this.proxyChain = new HttpHost[]{ proxy };
        this.secure     = secure;
    }

    





    public final void tunnelTarget(boolean secure) {
        if (!this.connected) {
            throw new IllegalStateException("No tunnel unless connected.");
        }
        if (this.proxyChain == null) {
            throw new IllegalStateException("No tunnel without proxy.");
        }
        this.tunnelled = TunnelType.TUNNELLED;
        this.secure    = secure;
    }

    








    public final void tunnelProxy(HttpHost proxy, boolean secure) {
        if (proxy == null) {
            throw new IllegalArgumentException("Proxy host may not be null.");
        }
        if (!this.connected) {
            throw new IllegalStateException("No tunnel unless connected.");
        }
        if (this.proxyChain == null) {
            throw new IllegalStateException("No proxy tunnel without proxy.");
        }

        
        HttpHost[] proxies = new HttpHost[this.proxyChain.length+1];
        System.arraycopy(this.proxyChain, 0,
                         proxies, 0, this.proxyChain.length);
        proxies[proxies.length-1] = proxy;

        this.proxyChain = proxies;
        this.secure     = secure;
    }

    





    public final void layerProtocol(boolean secure) {
        
        
        if (!this.connected) {
            throw new IllegalStateException
                ("No layered protocol unless connected.");
        }
        this.layered = LayerType.LAYERED;
        this.secure  = secure;
    }

    public final HttpHost getTargetHost() {
        return this.targetHost;
    }

    public final InetAddress getLocalAddress() {
        return this.localAddress;
    }

    public final int getHopCount() {
        int hops = 0;
        if (this.connected) {
            if (proxyChain == null)
                hops = 1;
            else
                hops = proxyChain.length + 1;
        }
        return hops;
    }

    public final HttpHost getHopTarget(int hop) {
        if (hop < 0)
            throw new IllegalArgumentException
                ("Hop index must not be negative: " + hop);
        final int hopcount = getHopCount();
        if (hop >= hopcount) {
            throw new IllegalArgumentException
                ("Hop index " + hop +
                 " exceeds tracked route length " + hopcount +".");
        }

        HttpHost result = null;
        if (hop < hopcount-1)
            result = this.proxyChain[hop];
        else
            result = this.targetHost;

        return result;
    }

    public final HttpHost getProxyHost() {
        return (this.proxyChain == null) ? null : this.proxyChain[0];
    }

    public final boolean isConnected() {
        return this.connected;
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

    







    public final HttpRoute toRoute() {
        return !this.connected ?
            null : new HttpRoute(this.targetHost, this.localAddress,
                                 this.proxyChain, this.secure,
                                 this.tunnelled, this.layered);
    }

    







    @Override
    public final boolean equals(Object o) {
        if (o == this)
            return true;
        if (!(o instanceof RouteTracker))
            return false;

        RouteTracker that = (RouteTracker) o;
        return
            
            (this.connected == that.connected) &&
            (this.secure    == that.secure) &&
            (this.tunnelled == that.tunnelled) &&
            (this.layered   == that.layered) &&
            LangUtils.equals(this.targetHost, that.targetHost) &&
            LangUtils.equals(this.localAddress, that.localAddress) &&
            LangUtils.equals(this.proxyChain, that.proxyChain);
    }

    







    @Override
    public final int hashCode() {
        int hash = LangUtils.HASH_SEED;
        hash = LangUtils.hashCode(hash, this.targetHost);
        hash = LangUtils.hashCode(hash, this.localAddress);
        if (this.proxyChain != null) {
            for (int i = 0; i < this.proxyChain.length; i++) {
                hash = LangUtils.hashCode(hash, this.proxyChain[i]);
            }
        }
        hash = LangUtils.hashCode(hash, this.connected);
        hash = LangUtils.hashCode(hash, this.secure);
        hash = LangUtils.hashCode(hash, this.tunnelled);
        hash = LangUtils.hashCode(hash, this.layered);
        return hash;
    }

    




    @Override
    public final String toString() {
        StringBuilder cab = new StringBuilder(50 + getHopCount()*30);

        cab.append("RouteTracker[");
        if (this.localAddress != null) {
            cab.append(this.localAddress);
            cab.append("->");
        }
        cab.append('{');
        if (this.connected)
            cab.append('c');
        if (this.tunnelled == TunnelType.TUNNELLED)
            cab.append('t');
        if (this.layered == LayerType.LAYERED)
            cab.append('l');
        if (this.secure)
            cab.append('s');
        cab.append("}->");
        if (this.proxyChain != null) {
            for (int i=0; i<this.proxyChain.length; i++) {
                cab.append(this.proxyChain[i]);
                cab.append("->");
            }
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
