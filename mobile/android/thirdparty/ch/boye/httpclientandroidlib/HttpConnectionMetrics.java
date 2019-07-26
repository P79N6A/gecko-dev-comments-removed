


























package ch.boye.httpclientandroidlib;






public interface HttpConnectionMetrics {

    



    long getRequestCount();

    



    long getResponseCount();

    



    long getSentBytesCount();

    



    long getReceivedBytesCount();

    







    Object getMetric(String metricName);

    



    void reset();

}
