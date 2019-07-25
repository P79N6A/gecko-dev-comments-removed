


























package ch.boye.httpclientandroidlib.client.methods;

import java.io.IOException;

import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionReleaseTrigger;
import ch.boye.httpclientandroidlib.conn.ManagedClientConnection;
import ch.boye.httpclientandroidlib.impl.conn.tsccm.ThreadSafeClientConnManager;









public interface AbortableHttpRequest {

    







    void setConnectionRequest(ClientConnectionRequest connRequest) throws IOException;

    





    void setReleaseTrigger(ConnectionReleaseTrigger releaseTrigger) throws IOException;

    













    void abort();

}

