

























package ch.boye.httpclientandroidlib.client.utils;

import ch.boye.httpclientandroidlib.annotation.Immutable;







@Immutable
public class Punycode {
    private static final Idn impl;
    static {
        Idn _impl;
        try {
            _impl = new JdkIdn();
        } catch (final Exception e) {
            _impl = new Rfc3492Idn();
        }
        impl = _impl;
    }

    public static String toUnicode(final String punycode) {
        return impl.toUnicode(punycode);
    }

}
