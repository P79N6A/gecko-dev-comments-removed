

























package ch.boye.httpclientandroidlib.impl.conn;

import java.net.InetAddress;
import java.net.UnknownHostException;

import ch.boye.httpclientandroidlib.conn.DnsResolver;






public class SystemDefaultDnsResolver implements DnsResolver {

    public static final SystemDefaultDnsResolver INSTANCE = new SystemDefaultDnsResolver();

    public InetAddress[] resolve(final String host) throws UnknownHostException {
        return InetAddress.getAllByName(host);
    }

}
