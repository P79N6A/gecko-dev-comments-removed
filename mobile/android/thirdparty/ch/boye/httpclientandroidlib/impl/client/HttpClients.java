


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.conn.HttpClientConnectionManager;
import ch.boye.httpclientandroidlib.impl.conn.PoolingHttpClientConnectionManager;





@Immutable
public class HttpClients {

    private HttpClients() {
        super();
    }

    



    public static HttpClientBuilder custom() {
        return HttpClientBuilder.create();
    }

    



    public static CloseableHttpClient createDefault() {
        return HttpClientBuilder.create().build();
    }

    



    public static CloseableHttpClient createSystem() {
        return HttpClientBuilder.create().useSystemProperties().build();
    }

    



    public static CloseableHttpClient createMinimal() {
        return new MinimalHttpClient(new PoolingHttpClientConnectionManager());
    }

    



    public static CloseableHttpClient createMinimal(final HttpClientConnectionManager connManager) {
        return new MinimalHttpClient(connManager);
    }

}
