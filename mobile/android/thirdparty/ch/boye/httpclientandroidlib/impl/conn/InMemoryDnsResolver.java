

























package ch.boye.httpclientandroidlib.impl.conn;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Arrays;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.conn.DnsResolver;
import ch.boye.httpclientandroidlib.util.Args;






public class InMemoryDnsResolver implements DnsResolver {

    
    public HttpClientAndroidLog log = new HttpClientAndroidLog(InMemoryDnsResolver.class);

    



    private final Map<String, InetAddress[]> dnsMap;

    



    public InMemoryDnsResolver() {
        dnsMap = new ConcurrentHashMap<String, InetAddress[]>();
    }

    









    public void add(final String host, final InetAddress... ips) {
        Args.notNull(host, "Host name");
        Args.notNull(ips, "Array of IP addresses");
        dnsMap.put(host, ips);
    }

    


    public InetAddress[] resolve(final String host) throws UnknownHostException {
        final InetAddress[] resolvedAddresses = dnsMap.get(host);
        if (log.isInfoEnabled()) {
            log.info("Resolving " + host + " to " + Arrays.deepToString(resolvedAddresses));
        }
        if(resolvedAddresses == null){
            throw new UnknownHostException(host + " cannot be resolved");
        }
        return resolvedAddresses;
    }

}
