


























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.entity.HttpEntityWrapper;
import ch.boye.httpclientandroidlib.util.EntityUtils;









@NotThreadSafe
public class BasicManagedEntity extends HttpEntityWrapper
    implements ConnectionReleaseTrigger, EofSensorWatcher {

    
    protected ManagedClientConnection managedConn;

    
    protected final boolean attemptReuse;

    









    public BasicManagedEntity(HttpEntity entity,
                              ManagedClientConnection conn,
                              boolean reuse) {
        super(entity);

        if (conn == null)
            throw new IllegalArgumentException
                ("Connection may not be null.");

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
        if (managedConn == null)
            return;

        try {
            if (attemptReuse) {
                
                EntityUtils.consume(wrappedEntity);
                managedConn.markReusable();
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

    public boolean eofDetected(InputStream wrapped) throws IOException {
        try {
            if (attemptReuse && (managedConn != null)) {
                
                
                wrapped.close();
                managedConn.markReusable();
            }
        } finally {
            releaseManagedConnection();
        }
        return false;
    }

    public boolean streamClosed(InputStream wrapped) throws IOException {
        try {
            if (attemptReuse && (managedConn != null)) {
                
                
                wrapped.close();
                managedConn.markReusable();
            }
        } finally {
            releaseManagedConnection();
        }
        return false;
    }

    public boolean streamAbort(InputStream wrapped) throws IOException {
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
