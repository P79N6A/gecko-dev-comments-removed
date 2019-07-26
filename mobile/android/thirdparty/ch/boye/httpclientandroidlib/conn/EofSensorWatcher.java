

























package ch.boye.httpclientandroidlib.conn;

import java.io.InputStream;
import java.io.IOException;







public interface EofSensorWatcher {

    












    boolean eofDetected(InputStream wrapped)
        throws IOException;

    














    boolean streamClosed(InputStream wrapped)
        throws IOException;

    

















    boolean streamAbort(InputStream wrapped)
        throws IOException;

}
