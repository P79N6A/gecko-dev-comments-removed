


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;




















@Deprecated
public interface ClientConnectionOperator {

    




    OperatedClientConnection createConnection();

    











    void openConnection(OperatedClientConnection conn,
                        HttpHost target,
                        InetAddress local,
                        HttpContext context,
                        HttpParams params)
        throws IOException;

    














    void updateSecureConnection(OperatedClientConnection conn,
                                HttpHost target,
                                HttpContext context,
                                HttpParams params)
        throws IOException;

}

