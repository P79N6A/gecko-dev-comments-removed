

























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.concurrent.TimeUnit;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpConnection;













@Deprecated
public class IdleConnectionHandler {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    
    private final Map<HttpConnection,TimeValues> connectionToTimes;


    public IdleConnectionHandler() {
        super();
        connectionToTimes = new HashMap<HttpConnection,TimeValues>();
    }

    







    public void add(final HttpConnection connection, final long validDuration, final TimeUnit unit) {

        final long timeAdded = System.currentTimeMillis();

        if (log.isDebugEnabled()) {
            log.debug("Adding connection at: " + timeAdded);
        }

        connectionToTimes.put(connection, new TimeValues(timeAdded, validDuration, unit));
    }

    







    public boolean remove(final HttpConnection connection) {
        final TimeValues times = connectionToTimes.remove(connection);
        if(times == null) {
            log.warn("Removing a connection that never existed!");
            return true;
        } else {
            return System.currentTimeMillis() <= times.timeExpires;
        }
    }

    


    public void removeAll() {
        this.connectionToTimes.clear();
    }

    




    public void closeIdleConnections(final long idleTime) {

        
        final long idleTimeout = System.currentTimeMillis() - idleTime;

        if (log.isDebugEnabled()) {
            log.debug("Checking for connections, idle timeout: "  + idleTimeout);
        }

        for (final Entry<HttpConnection, TimeValues> entry : connectionToTimes.entrySet()) {
            final HttpConnection conn = entry.getKey();
            final TimeValues times = entry.getValue();
            final long connectionTime = times.timeAdded;
            if (connectionTime <= idleTimeout) {
                if (log.isDebugEnabled()) {
                    log.debug("Closing idle connection, connection time: "  + connectionTime);
                }
                try {
                    conn.close();
                } catch (final IOException ex) {
                    log.debug("I/O error closing connection", ex);
                }
            }
        }
    }


    public void closeExpiredConnections() {
        final long now = System.currentTimeMillis();
        if (log.isDebugEnabled()) {
            log.debug("Checking for expired connections, now: "  + now);
        }

        for (final Entry<HttpConnection, TimeValues> entry : connectionToTimes.entrySet()) {
            final HttpConnection conn = entry.getKey();
            final TimeValues times = entry.getValue();
            if(times.timeExpires <= now) {
                if (log.isDebugEnabled()) {
                    log.debug("Closing connection, expired @: "  + times.timeExpires);
                }
                try {
                    conn.close();
                } catch (final IOException ex) {
                    log.debug("I/O error closing connection", ex);
                }
            }
        }
    }

    private static class TimeValues {
        private final long timeAdded;
        private final long timeExpires;

        




        TimeValues(final long now, final long validDuration, final TimeUnit validUnit) {
            this.timeAdded = now;
            if(validDuration > 0) {
                this.timeExpires = now + validUnit.toMillis(validDuration);
            } else {
                this.timeExpires = Long.MAX_VALUE;
            }
        }
    }
}
