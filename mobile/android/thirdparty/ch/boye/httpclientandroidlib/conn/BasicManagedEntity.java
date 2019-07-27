

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.SocketException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.EntityUtils;











@Deprecated
@NotThreadSafe
public class BasicManagedEntity extends HttpEntityWrapper
    implements ConnectionReleaseTrigger, EofSensorWatcher {

    
    protected ManagedClientConnection managedConn;

    
    protected final boolean attemptReuse;

    









    public BasicManagedEntity(final HttpEntity entity,
                              final ManagedClientConnection conn,
                              final boolean reuse) {
        super(entity);
        Args.notNull(conn, "Connection");
        this.managedConn = conn;
        this.attemptReuse = reuse;
    }

    @Override
    public boolean isRepeatable() {
        return false;
    }

    @Override
    public InputStream getContent() throws IOException {
        return new EofSensorInputStream(wrappedEntity.getContent(), this);
    }

    private void ensureConsumed() throws IOException {
        if (managedConn == null) {
            return;
        }

        try {
            if (attemptReuse) {
                
                EntityUtils.consume(wrappedEntity);
                managedConn.markReusable();
            } else {
                managedConn.unmarkReusable();
            }
        } finally {
            releaseManagedConnection();
        }
    }

    


    @Deprecated
    @Override
    public void consumeContent() throws IOException {
        ensureConsumed();
    }

    @Override
    public void writeTo(final OutputStream outstream) throws IOException {
        super.writeTo(outstream);
        ensureConsumed();
    }

    public void releaseConnection() throws IOException {
        ensureConsumed();
    }

    public void abortConnection() throws IOException {

        if (managedConn != null) {
            try {
                managedConn.abortConnection();
            } finally {
                managedConn = null;
            }
        }
    }

    public boolean eofDetected(final InputStream wrapped) throws IOException {
        try {
            if (managedConn != null) {
                if (attemptReuse) {
                    
                    
                    wrapped.close();
                    managedConn.markReusable();
                } else {
                    managedConn.unmarkReusable();
                }
            }
        } finally {
            releaseManagedConnection();
        }
        return false;
    }

    public boolean streamClosed(final InputStream wrapped) throws IOException {
        try {
            if (managedConn != null) {
                if (attemptReuse) {
                    final boolean valid = managedConn.isOpen();
                    
                    
                    try {
                        wrapped.close();
                        managedConn.markReusable();
                    } catch (final SocketException ex) {
                        if (valid) {
                            throw ex;
                        }
                    }
                } else {
                    managedConn.unmarkReusable();
                }
            }
        } finally {
            releaseManagedConnection();
        }
        return false;
    }

    public boolean streamAbort(final InputStream wrapped) throws IOException {
        if (managedConn != null) {
            managedConn.abortConnection();
        }
        return false;
    }

    







    protected void releaseManagedConnection()
        throws IOException {

        if (managedConn != null) {
            try {
                managedConn.releaseConnection();
            } finally {
                managedConn = null;
            }
        }
    }

}
