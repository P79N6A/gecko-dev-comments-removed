

























package ch.boye.httpclientandroidlib.conn;

import java.io.IOException;
import java.io.InputStream;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;












@NotThreadSafe
public class EofSensorInputStream extends InputStream implements ConnectionReleaseTrigger {

    




    protected InputStream wrappedStream;

    









    private boolean selfClosed;

    
    private final EofSensorWatcher eofWatcher;

    










    public EofSensorInputStream(final InputStream in,
                                final EofSensorWatcher watcher) {
        Args.notNull(in, "Wrapped stream");
        wrappedStream = in;
        selfClosed = false;
        eofWatcher = watcher;
    }

    boolean isSelfClosed() {
        return selfClosed;
    }

    InputStream getWrappedStream() {
        return wrappedStream;
    }

    








    protected boolean isReadAllowed() throws IOException {
        if (selfClosed) {
            throw new IOException("Attempted read on closed stream.");
        }
        return (wrappedStream != null);
    }

    @Override
    public int read() throws IOException {
        int l = -1;

        if (isReadAllowed()) {
            try {
                l = wrappedStream.read();
                checkEOF(l);
            } catch (final IOException ex) {
                checkAbort();
                throw ex;
            }
        }

        return l;
    }

    @Override
    public int read(final byte[] b, final int off, final int len) throws IOException {
        int l = -1;

        if (isReadAllowed()) {
            try {
                l = wrappedStream.read(b,  off,  len);
                checkEOF(l);
            } catch (final IOException ex) {
                checkAbort();
                throw ex;
            }
        }

        return l;
    }

    @Override
    public int read(final byte[] b) throws IOException {
        return read(b, 0, b.length);
    }

    @Override
    public int available() throws IOException {
        int a = 0; 

        if (isReadAllowed()) {
            try {
                a = wrappedStream.available();
                
            } catch (final IOException ex) {
                checkAbort();
                throw ex;
            }
        }

        return a;
    }

    @Override
    public void close() throws IOException {
        
        selfClosed = true;
        checkClose();
    }

    















    protected void checkEOF(final int eof) throws IOException {

        if ((wrappedStream != null) && (eof < 0)) {
            try {
                boolean scws = true; 
                if (eofWatcher != null) {
                    scws = eofWatcher.eofDetected(wrappedStream);
                }
                if (scws) {
                    wrappedStream.close();
                }
            } finally {
                wrappedStream = null;
            }
        }
    }

    










    protected void checkClose() throws IOException {

        if (wrappedStream != null) {
            try {
                boolean scws = true; 
                if (eofWatcher != null) {
                    scws = eofWatcher.streamClosed(wrappedStream);
                }
                if (scws) {
                    wrappedStream.close();
                }
            } finally {
                wrappedStream = null;
            }
        }
    }

    












    protected void checkAbort() throws IOException {

        if (wrappedStream != null) {
            try {
                boolean scws = true; 
                if (eofWatcher != null) {
                    scws = eofWatcher.streamAbort(wrappedStream);
                }
                if (scws) {
                    wrappedStream.close();
                }
            } finally {
                wrappedStream = null;
            }
        }
    }

    


    public void releaseConnection() throws IOException {
        close();
    }

    






    public void abortConnection() throws IOException {
        
        selfClosed = true;
        checkAbort();
    }

}

