


























package ch.boye.httpclientandroidlib.impl;

import java.util.HashMap;
import java.util.Map;

import ch.boye.httpclientandroidlib.HttpConnectionMetrics;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.HttpTransportMetrics;






@NotThreadSafe
public class HttpConnectionMetricsImpl implements HttpConnectionMetrics {

    public static final String REQUEST_COUNT = "http.request-count";
    public static final String RESPONSE_COUNT = "http.response-count";
    public static final String SENT_BYTES_COUNT = "http.sent-bytes-count";
    public static final String RECEIVED_BYTES_COUNT = "http.received-bytes-count";

    private final HttpTransportMetrics inTransportMetric;
    private final HttpTransportMetrics outTransportMetric;
    private long requestCount = 0;
    private long responseCount = 0;

    


    private Map<String, Object> metricsCache;

    public HttpConnectionMetricsImpl(
            final HttpTransportMetrics inTransportMetric,
            final HttpTransportMetrics outTransportMetric) {
        super();
        this.inTransportMetric = inTransportMetric;
        this.outTransportMetric = outTransportMetric;
    }

    

    public long getReceivedBytesCount() {
        if (this.inTransportMetric != null) {
            return this.inTransportMetric.getBytesTransferred();
        } else {
            return -1;
        }
    }

    public long getSentBytesCount() {
        if (this.outTransportMetric != null) {
            return this.outTransportMetric.getBytesTransferred();
        } else {
            return -1;
        }
    }

    public long getRequestCount() {
        return this.requestCount;
    }

    public void incrementRequestCount() {
        this.requestCount++;
    }

    public long getResponseCount() {
        return this.responseCount;
    }

    public void incrementResponseCount() {
        this.responseCount++;
    }

    public Object getMetric(final String metricName) {
        Object value = null;
        if (this.metricsCache != null) {
            value = this.metricsCache.get(metricName);
        }
        if (value == null) {
            if (REQUEST_COUNT.equals(metricName)) {
                value = Long.valueOf(requestCount);
            } else if (RESPONSE_COUNT.equals(metricName)) {
                value = Long.valueOf(responseCount);
            } else if (RECEIVED_BYTES_COUNT.equals(metricName)) {
                if (this.inTransportMetric != null) {
                    return Long.valueOf(this.inTransportMetric.getBytesTransferred());
                } else {
                    return null;
                }
            } else if (SENT_BYTES_COUNT.equals(metricName)) {
                if (this.outTransportMetric != null) {
                    return Long.valueOf(this.outTransportMetric.getBytesTransferred());
                } else {
                    return null;
                }
            }
        }
        return value;
    }

    public void setMetric(final String metricName, final Object obj) {
        if (this.metricsCache == null) {
            this.metricsCache = new HashMap<String, Object>();
        }
        this.metricsCache.put(metricName, obj);
    }

    public void reset() {
        if (this.outTransportMetric != null) {
            this.outTransportMetric.reset();
        }
        if (this.inTransportMetric != null) {
            this.inTransportMetric.reset();
        }
        this.requestCount = 0;
        this.responseCount = 0;
        this.metricsCache = null;
    }

}
