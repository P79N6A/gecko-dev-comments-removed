

















package org.mozilla.gecko.util;

import android.text.TextUtils;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Proxy;

public class ProxySelector {
    public ProxySelector() {
    }

    public Proxy select(String scheme, String host) {
        int port = -1;
        Proxy proxy = null;
        String nonProxyHostsKey = null;
        boolean httpProxyOkay = true;
        if ("http".equalsIgnoreCase(scheme)) {
            port = 80;
            nonProxyHostsKey = "http.nonProxyHosts";
            proxy = lookupProxy("http.proxyHost", "http.proxyPort", Proxy.Type.HTTP, port);
        } else if ("https".equalsIgnoreCase(scheme)) {
            port = 443;
            nonProxyHostsKey = "https.nonProxyHosts"; 
            proxy = lookupProxy("https.proxyHost", "https.proxyPort", Proxy.Type.HTTP, port);
        } else if ("ftp".equalsIgnoreCase(scheme)) {
            port = 80; 
            nonProxyHostsKey = "ftp.nonProxyHosts";
            proxy = lookupProxy("ftp.proxyHost", "ftp.proxyPort", Proxy.Type.HTTP, port);
        } else if ("socket".equalsIgnoreCase(scheme)) {
            httpProxyOkay = false;
        } else {
            return Proxy.NO_PROXY;
        }

        if (nonProxyHostsKey != null
                && isNonProxyHost(host, System.getProperty(nonProxyHostsKey))) {
            return Proxy.NO_PROXY;
        }

        if (proxy != null) {
            return proxy;
        }

        if (httpProxyOkay) {
            proxy = lookupProxy("proxyHost", "proxyPort", Proxy.Type.HTTP, port);
            if (proxy != null) {
                return proxy;
            }
        }

        proxy = lookupProxy("socksProxyHost", "socksProxyPort", Proxy.Type.SOCKS, 1080);
        if (proxy != null) {
            return proxy;
        }

        return Proxy.NO_PROXY;
    }

    



    private Proxy lookupProxy(String hostKey, String portKey, Proxy.Type type, int defaultPort) {
        String host = System.getProperty(hostKey);
        if (TextUtils.isEmpty(host)) {
            return null;
        }

        int port = getSystemPropertyInt(portKey, defaultPort);
        return new Proxy(type, InetSocketAddress.createUnresolved(host, port));
    }

    private int getSystemPropertyInt(String key, int defaultValue) {
        String string = System.getProperty(key);
        if (string != null) {
            try {
                return Integer.parseInt(string);
            } catch (NumberFormatException ignored) {
            }
        }
        return defaultValue;
    }

    



    private boolean isNonProxyHost(String host, String nonProxyHosts) {
        if (host == null || nonProxyHosts == null) {
            return false;
        }

        
        StringBuilder patternBuilder = new StringBuilder();
        for (int i = 0; i < nonProxyHosts.length(); i++) {
            char c = nonProxyHosts.charAt(i);
            switch (c) {
            case '.':
                patternBuilder.append("\\.");
                break;
            case '*':
                patternBuilder.append(".*");
                break;
            default:
                patternBuilder.append(c);
            }
        }
        
        String pattern = patternBuilder.toString();
        return host.matches(pattern);
    }
}

