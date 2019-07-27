


























package ch.boye.httpclientandroidlib.conn.ssl;

import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;

import javax.net.ssl.SSLContext;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public class SSLContexts {

    






    public static SSLContext createDefault() throws SSLInitializationException {
        try {
            final SSLContext sslcontext = SSLContext.getInstance(SSLContextBuilder.TLS);
            sslcontext.init(null, null, null);
            return sslcontext;
        } catch (final NoSuchAlgorithmException ex) {
            throw new SSLInitializationException(ex.getMessage(), ex);
        } catch (final KeyManagementException ex) {
            throw new SSLInitializationException(ex.getMessage(), ex);
        }
    }

    








    public static SSLContext createSystemDefault() throws SSLInitializationException {
        try {
            return SSLContext.getInstance("Default");
        } catch (final NoSuchAlgorithmException ex) {
            return createDefault();
        }
    }

    




    public static SSLContextBuilder custom() {
        return new SSLContextBuilder();
    }

}
