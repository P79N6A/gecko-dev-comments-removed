

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;









@Deprecated
@NotThreadSafe
public class BasicEofSensorWatcher implements EofSensorWatcher {

    
    protected final ManagedClientConnection managedConn;

    
    protected final boolean attemptReuse;

    





    public BasicEofSensorWatcher(final ManagedClientConnection conn,
                                 final boolean reuse) {
        Args.notNull(conn, "Connection");
        managedConn = conn;
        attemptReuse = reuse;
    }

    public boolean eofDetected(final InputStream wrapped)
        throws IOException {

        try {
            if (attemptReuse) {
                
                
                wrapped.close();
                managedConn.markReusable();
            }
        } finally {
            managedConn.releaseConnection();
        }
        return false;
    }

    public boolean streamClosed(final InputStream wrapped)
        throws IOException {

        try {
            if (attemptReuse) {
                
                
                wrapped.close();
                managedConn.markReusable();
            }
        } finally {
            managedConn.releaseConnection();
        }
        return false;
    }

    public boolean streamAbort(final InputStream wrapped)
        throws IOException {

        managedConn.abortConnection();
        return false;
    }

}
