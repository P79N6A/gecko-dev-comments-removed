




































package org.mozilla.gecko.sync.crypto;

import java.io.UnsupportedEncodingException;
import java.util.Arrays;

import org.mozilla.apache.commons.codec.binary.Base32;
import org.mozilla.apache.commons.codec.binary.Base64;

public class Utils {

    




    public static byte[] hex2Byte(String str)
    {
        if (str.length() % 2 == 1) {
            str = "0" + str;
        }

        byte[] bytes = new byte[str.length() / 2];
        for (int i = 0; i < bytes.length; i++)
        {
            bytes[i] = (byte) Integer
                .parseInt(str.substring(2 * i, 2 * i + 2), 16);
        }
        return bytes;
    }

    




    public static String byte2hex(byte[] b) {

        
        String hs = "";
        String stmp = "";

        for (int n = 0; n < b.length; n++) {
            stmp = (java.lang.Integer.toHexString(b[n] & 0XFF));

            if (stmp.length() == 1) {
                hs = hs + "0" + stmp;
            } else {
                hs = hs + stmp;
            }

            if (n < b.length - 1) {
                hs = hs + "";
            }
        }

        return hs;
    }

    




    public static byte[] concatAll(byte[] first, byte[]... rest) {
        int totalLength = first.length;
        for (byte[] array : rest) {
            totalLength += array.length;
        }

        byte[] result = Arrays.copyOf(first, totalLength);
        int offset = first.length;

        for (byte[] array : rest) {
            System.arraycopy(array, 0, result, offset, array.length);
            offset += array.length;
        }
        return result;
    }

    


    public static byte[] decodeFriendlyBase32(String base32) {
        Base32 converter = new Base32();
        return converter.decode(base32.replace('8', 'l').replace('9', 'o')
                .toUpperCase());
    }

    










    public static byte[] decodeBase64(String base64) throws UnsupportedEncodingException {
        return Base64.decodeBase64(base64.getBytes("UTF-8"));
    }
}
