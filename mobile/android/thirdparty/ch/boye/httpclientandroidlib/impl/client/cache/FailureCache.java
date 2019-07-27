

























package ch.boye.httpclientandroidlib.impl.client.cache;







public interface FailureCache {

    




    int getErrorCount(String identifier);

    




    void resetErrorCount(String identifier);

    




    void increaseErrorCount(String identifier);
}
