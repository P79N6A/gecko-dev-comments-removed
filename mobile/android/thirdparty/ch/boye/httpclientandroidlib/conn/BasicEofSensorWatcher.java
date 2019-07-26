


























package ch.boye.httpclientandroidlib.conn;

import java.io.InputStream;
import java.io.IOException;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;







@NotThreadSafe
public class BasicEofSensorWatcher implements EofSensorWatcher {

    
    protected final ManagedClientConnection managedConn;

    
    protected final boolean attemptReuse;

    





    public BasicEofSensorWatcher(ManagedClientConnection conn,
                                 boolean reuse) {
        if (conn == null)
            throw new IllegalArgumentException
                ("Connection may not be null.");

        managedConn = conn;
        attemptReuse = reuse;
    }

    public boolean eofDetected(InputStream wrapped)
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

    public boolean streamClosed(InputStream wrapped)
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

    public boolean streamAbort(InputStream wrapped)
        throws IOException {

        managedConn.abortConnection();
        return false;
    }

}
