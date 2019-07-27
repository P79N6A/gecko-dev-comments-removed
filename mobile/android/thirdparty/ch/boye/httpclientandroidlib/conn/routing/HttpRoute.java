


























package ch.boye.httpclientandroidlib.conn.routing;

import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.LangUtils;






@Immutable
public final class HttpRoute implements RouteInfo, Cloneable {

    
    private final HttpHost targetHost;

    



    private final InetAddress localAddress;

    
    private final List<HttpHost> proxyChain;

    
    private final TunnelType tunnelled;

    
    private final LayerType layered;

    
    private final boolean secure;

    private HttpRoute(final HttpHost target, final InetAddress local, final List<HttpHost> proxies,
                     final boolean secure, final TunnelType tunnelled, final LayerType layered) {
        Args.notNull(target, "Target host");
        this.targetHost   = target;
        this.localAddress = local;
        if (proxies != null && !proxies.isEmpty()) {
            this.proxyChain = new ArrayList<HttpHost>(proxies);
        } else {
            this.proxyChain = null;
        }
        if (tunnelled == TunnelType.TUNNELLED) {
            Args.check(this.proxyChain != null, "Proxy required if tunnelled");
        }
        this.secure       = secure;
        this.tunnelled    = tunnelled != null ? tunnelled : TunnelType.PLAIN;
        this.layered      = layered != null ? layered : LayerType.PLAIN;
    }

    












    public HttpRoute(final HttpHost target, final InetAddress local, final HttpHost[] proxies,
                     final boolean secure, final TunnelType tunnelled, final LayerType layered) {
        this(target, local, proxies != null ? Arrays.asList(proxies) : null,
                secure, tunnelled, layered);
    }

    
















    public HttpRoute(final HttpHost target, final InetAddress local, final HttpHost proxy,
                     final boolean secure, final TunnelType tunnelled, final LayerType layered) {
        this(target, local, proxy != null ? Collections.singletonList(proxy) : null,
                secure, tunnelled, layered);
    }

    









    public HttpRoute(final HttpHost target, final InetAddress local, final boolean secure) {
        this(target, local, Collections.<HttpHost>emptyList(), secure,
                TunnelType.PLAIN, LayerType.PLAIN);
    }

    




    public HttpRoute(final HttpHost target) {
        this(target, null, Collections.<HttpHost>emptyList(), false,
                TunnelType.PLAIN, LayerType.PLAIN);
    }

    












    public HttpRoute(final HttpHost target, final InetAddress local, final HttpHost proxy,
                     final boolean secure) {
        this(target, local, Collections.singletonList(Args.notNull(proxy, "Proxy host")), secure,
             secure ? TunnelType.TUNNELLED : TunnelType.PLAIN,
             secure ? LayerType.LAYERED    : LayerType.PLAIN);
    }

    







    public HttpRoute(final HttpHost target, final HttpHost proxy) {
        this(target, null, proxy, false);
    }

    public final HttpHost getTargetHost() {
        return this.targetHost;
    }

    public final InetAddress getLocalAddress() {
        return this.localAddress;
    }

    public final InetSocketAddress getLocalSocketAddress() {
        return this.localAddress != null ? new InetSocketAddress(this.localAddress, 0) : null;
    }

    public final int getHopCount() {
        return proxyChain != null ? proxyChain.size() + 1 : 1;
    }

    public final HttpHost getHopTarget(final int hop) {
        Args.notNegative(hop, "Hop index");
        final int hopcount = getHopCount();
        Args.check(hop < hopcount, "Hop index exceeds tracked route length");
        if (hop < hopcount - 1) {
            return this.proxyChain.get(hop);
        } else {
            return this.targetHost;
        }
    }

    public final HttpHost getProxyHost() {
        return proxyChain != null && !this.proxyChain.isEmpty() ? this.proxyChain.get(0) : null;
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
    public final boolean equals(final Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof HttpRoute) {
            final HttpRoute that = (HttpRoute) obj;
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
        if (this.proxyChain != null) {
            for (final HttpHost element : this.proxyChain) {
                hash = LangUtils.hashCode(hash, element);
            }
        }
        hash = LangUtils.hashCode(hash, this.secure);
        hash = LangUtils.hashCode(hash, this.tunnelled);
        hash = LangUtils.hashCode(hash, this.layered);
        return hash;
    }

    




    @Override
    public final String toString() {
        final StringBuilder cab = new StringBuilder(50 + getHopCount()*30);
        if (this.localAddress != null) {
            cab.append(this.localAddress);
            cab.append("->");
        }
        cab.append('{');
        if (this.tunnelled == TunnelType.TUNNELLED) {
            cab.append('t');
        }
        if (this.layered == LayerType.LAYERED) {
            cab.append('l');
        }
        if (this.secure) {
            cab.append('s');
        }
        cab.append("}->");
        if (this.proxyChain != null) {
            for (final HttpHost aProxyChain : this.proxyChain) {
                cab.append(aProxyChain);
                cab.append("->");
            }
        }
        cab.append(this.targetHost);
        return cab.toString();
    }

    
    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
