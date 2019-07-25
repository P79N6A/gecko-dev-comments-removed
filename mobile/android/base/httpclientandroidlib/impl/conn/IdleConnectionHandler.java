
























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

    







    public void add(HttpConnection connection, long validDuration, TimeUnit unit) {

        long timeAdded = System.currentTimeMillis();

        if (log.isDebugEnabled()) {
            log.debug("Adding connection at: " + timeAdded);
        }

        connectionToTimes.put(connection, new TimeValues(timeAdded, validDuration, unit));
    }

    







    public boolean remove(HttpConnection connection) {
        TimeValues times = connectionToTimes.remove(connection);
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

    




    public void closeIdleConnections(long idleTime) {

        
        long idleTimeout = System.currentTimeMillis() - idleTime;

        if (log.isDebugEnabled()) {
            log.debug("Checking for connections, idle timeout: "  + idleTimeout);
        }

        for (Entry<HttpConnection, TimeValues> entry : connectionToTimes.entrySet()) {
            HttpConnection conn = entry.getKey();
            TimeValues times = entry.getValue();
            long connectionTime = times.timeAdded;
            if (connectionTime <= idleTimeout) {
                if (log.isDebugEnabled()) {
                    log.debug("Closing idle connection, connection time: "  + connectionTime);
                }
                try {
                    conn.close();
                } catch (IOException ex) {
                    log.debug("I/O error closing connection", ex);
                }
            }
        }
    }


    public void closeExpiredConnections() {
        long now = System.currentTimeMillis();
        if (log.isDebugEnabled()) {
            log.debug("Checking for expired connections, now: "  + now);
        }

        for (Entry<HttpConnection, TimeValues> entry : connectionToTimes.entrySet()) {
            HttpConnection conn = entry.getKey();
            TimeValues times = entry.getValue();
            if(times.timeExpires <= now) {
                if (log.isDebugEnabled()) {
                    log.debug("Closing connection, expired @: "  + times.timeExpires);
                }
                try {
                    conn.close();
                } catch (IOException ex) {
                    log.debug("I/O error closing connection", ex);
                }
            }
        }
    }

    private static class TimeValues {
        private final long timeAdded;
        private final long timeExpires;

        




        TimeValues(long now, long validDuration, TimeUnit validUnit) {
            this.timeAdded = now;
            if(validDuration > 0) {
                this.timeExpires = now + validUnit.toMillis(validDuration);
            } else {
                this.timeExpires = Long.MAX_VALUE;
            }
        }
    }
}
