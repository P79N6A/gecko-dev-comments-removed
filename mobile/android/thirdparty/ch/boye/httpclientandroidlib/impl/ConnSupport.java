

























package ch.boye.httpclientandroidlib.impl;

import ch.boye.httpclientandroidlib.config.ConnectionConfig;

import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CodingErrorAction;






public final class ConnSupport {

    public static CharsetDecoder createDecoder(final ConnectionConfig cconfig) {
        if (cconfig == null) {
            return null;
        }
        final Charset charset = cconfig.getCharset();
        final CodingErrorAction malformed = cconfig.getMalformedInputAction();
        final CodingErrorAction unmappable = cconfig.getUnmappableInputAction();
        if (charset != null) {
            return charset.newDecoder()
                    .onMalformedInput(malformed != null ? malformed : CodingErrorAction.REPORT)
                    .onUnmappableCharacter(unmappable != null ? unmappable: CodingErrorAction.REPORT);
        } else {
            return null;
        }
    }

    public static CharsetEncoder createEncoder(final ConnectionConfig cconfig) {
        if (cconfig == null) {
            return null;
        }
        final Charset charset = cconfig.getCharset();
        if (charset != null) {
            final CodingErrorAction malformed = cconfig.getMalformedInputAction();
            final CodingErrorAction unmappable = cconfig.getUnmappableInputAction();
            return charset.newEncoder()
                .onMalformedInput(malformed != null ? malformed : CodingErrorAction.REPORT)
                .onUnmappableCharacter(unmappable != null ? unmappable: CodingErrorAction.REPORT);
        } else {
            return null;
        }
    }

}
