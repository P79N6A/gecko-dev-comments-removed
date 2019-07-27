


























package ch.boye.httpclientandroidlib.client.methods;

import ch.boye.httpclientandroidlib.conn.ClientConnectionRequest;
import ch.boye.httpclientandroidlib.conn.ConnectionReleaseTrigger;

import java.io.IOException;










@Deprecated
public interface AbortableHttpRequest {

    






    void setConnectionRequest(ClientConnectionRequest connRequest) throws IOException;

    






    void setReleaseTrigger(ConnectionReleaseTrigger releaseTrigger) throws IOException;

    













    void abort();

}

