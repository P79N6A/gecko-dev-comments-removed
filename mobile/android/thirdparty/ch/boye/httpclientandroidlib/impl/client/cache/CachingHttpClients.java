


























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.io.File;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.impl.client.CloseableHttpClient;







@Immutable
public class CachingHttpClients {

    private CachingHttpClients() {
        super();
    }

    



    public static CachingHttpClientBuilder custom() {
        return CachingHttpClientBuilder.create();
    }

    



    public static CloseableHttpClient createMemoryBound() {
        return CachingHttpClientBuilder.create().build();
    }

    





    public static CloseableHttpClient createFileBound(final File cacheDir) {
        return CachingHttpClientBuilder.create().setCacheDir(cacheDir).build();
    }

}
