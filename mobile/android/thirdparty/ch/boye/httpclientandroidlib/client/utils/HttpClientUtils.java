

























package ch.boye.httpclientandroidlib.client.utils;

import java.io.Closeable;
import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.HttpClient;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.util.EntityUtils;






public class HttpClientUtils {

    private HttpClientUtils() {
    }

    





















    public static void closeQuietly(final HttpResponse response) {
        if (response != null) {
            final HttpEntity entity = response.getEntity();
            if (entity != null) {
                try {
                    EntityUtils.consume(entity);
                } catch (final IOException ex) {
                }
            }
        }
    }

    





















    public static void closeQuietly(final CloseableHttpResponse response) {
        if (response != null) {
            try {
                try {
                    EntityUtils.consume(response.getEntity());
                } finally {
                    response.close();
                }
            } catch (final IOException ignore) {
            }
        }
    }

    




















    public static void closeQuietly(final HttpClient httpClient) {
        if (httpClient != null) {
            if (httpClient instanceof Closeable) {
                try {
                    ((Closeable) httpClient).close();
                } catch (final IOException ignore) {
                }
            }
        }
    }

}
