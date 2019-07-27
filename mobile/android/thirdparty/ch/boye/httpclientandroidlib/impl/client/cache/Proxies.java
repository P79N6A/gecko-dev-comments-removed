

























package ch.boye.httpclientandroidlib.impl.client.cache;

import java.lang.reflect.Proxy;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.util.Args;






@NotThreadSafe
class Proxies {

    public static CloseableHttpResponse enhanceResponse(final HttpResponse original) {
        Args.notNull(original, "HTTP response");
        if (original instanceof CloseableHttpResponse) {
            return (CloseableHttpResponse) original;
        } else {
            return (CloseableHttpResponse) Proxy.newProxyInstance(
                    ResponseProxyHandler.class.getClassLoader(),
                    new Class<?>[] { CloseableHttpResponse.class },
                    new ResponseProxyHandler(original));
        }
    }

}
