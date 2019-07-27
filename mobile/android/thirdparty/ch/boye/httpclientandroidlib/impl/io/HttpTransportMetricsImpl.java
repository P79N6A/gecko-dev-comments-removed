


























package ch.boye.httpclientandroidlib.impl.io;

import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;






@NotThreadSafe
public class HttpTransportMetricsImpl implements HttpTransportMetrics {

    private long bytesTransferred = 0;

    public HttpTransportMetricsImpl() {
        super();
    }

    public long getBytesTransferred() {
        return this.bytesTransferred;
    }

    public void setBytesTransferred(final long count) {
        this.bytesTransferred = count;
    }

    public void incrementBytesTransferred(final long count) {
        this.bytesTransferred += count;
    }

    public void reset() {
        this.bytesTransferred = 0;
    }

}
