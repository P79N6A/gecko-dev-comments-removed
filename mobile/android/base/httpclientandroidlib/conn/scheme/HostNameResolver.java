


























package ch.boye.httpclientandroidlib.conn.scheme;

import java.io.IOException;
import java.net.InetAddress;








@Deprecated
public interface HostNameResolver {

    






    InetAddress resolve (String hostname) throws IOException;

}
