

















package org.mozilla.apache.commons.codec.net;

import org.mozilla.apache.commons.codec.DecoderException;








class Utils {

    









    static int digit16(byte b) throws DecoderException {
        int i = Character.digit((char) b, 16);
        if (i == -1) {
            throw new DecoderException("Invalid URL encoding: not a valid digit (radix " + URLCodec.RADIX + "): " + b);
        }
        return i;
    }

}
