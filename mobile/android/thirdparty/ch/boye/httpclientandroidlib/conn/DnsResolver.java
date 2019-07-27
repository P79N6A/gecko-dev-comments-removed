

























package ch.boye.httpclientandroidlib.conn;

import java.net.InetAddress;
import java.net.UnknownHostException;







public interface DnsResolver {

    











    InetAddress[] resolve(String host) throws UnknownHostException;

}
