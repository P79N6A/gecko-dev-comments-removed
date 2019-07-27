

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InputStream;







public interface EofSensorWatcher {

    












    boolean eofDetected(InputStream wrapped)
        throws IOException;

    














    boolean streamClosed(InputStream wrapped)
        throws IOException;

    

















    boolean streamAbort(InputStream wrapped)
        throws IOException;

}
