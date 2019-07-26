

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;













public interface ConnectionReleaseTrigger {

    










    void releaseConnection()
        throws IOException;

    







    void abortConnection()
        throws IOException;

}
