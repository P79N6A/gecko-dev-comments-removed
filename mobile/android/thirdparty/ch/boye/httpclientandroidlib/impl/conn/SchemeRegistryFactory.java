

























package ch.boye.httpclientandroidlib.impl.conn;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.conn.scheme.PlainSocketFactory;
import ch.boye.httpclientandroidlib.conn.scheme.Scheme;
import ch.boye.httpclientandroidlib.conn.scheme.SchemeRegistry;
import ch.boye.httpclientandroidlib.conn.ssl.SSLSocketFactory;






@ThreadSafe
@Deprecated
public final class SchemeRegistryFactory {

    



    public static SchemeRegistry createDefault() {
        final SchemeRegistry registry = new SchemeRegistry();
        registry.register(
                new Scheme("http", 80, PlainSocketFactory.getSocketFactory()));
        registry.register(
                new Scheme("https", 443, SSLSocketFactory.getSocketFactory()));
        return registry;
    }

    























    public static SchemeRegistry createSystemDefault() {
        final SchemeRegistry registry = new SchemeRegistry();
        registry.register(
                new Scheme("http", 80, PlainSocketFactory.getSocketFactory()));
        registry.register(
                new Scheme("https", 443, SSLSocketFactory.getSystemSocketFactory()));
        return registry;
    }
}

