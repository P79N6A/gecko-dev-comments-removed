




































package org.mozilla.gecko.sync.crypto;

import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

import org.mozilla.gecko.sync.Utils;







public class HKDF {

    


    public static final byte[] bytes(String in) {
      try {
          return in.getBytes("UTF-8");
      } catch (java.io.UnsupportedEncodingException e) {
          return null;
      }
    }

    public static final int BLOCKSIZE     = 256 / 8;
    public static final byte[] HMAC_INPUT = bytes("Sync-AES_256_CBC-HMAC256");

    





    public static byte[] hkdfExtract(byte[] salt, byte[] IKM) {
        return digestBytes(IKM, makeHMACHasher(salt));
    }

    




    public static byte[] hkdfExpand(byte[] prk, byte[] info, int len) {

        Mac hmacHasher = makeHMACHasher(prk);

        byte[] T  = {};
        byte[] Tn = {};

        int iterations = (int) Math.ceil(((double)len) / ((double)BLOCKSIZE));
        for (int i = 0; i < iterations; i++) {
            Tn = digestBytes(Utils.concatAll
                    (Tn, info, Utils.hex2Byte(Integer.toHexString(i + 1))), hmacHasher);
            T = Utils.concatAll(T, Tn);
        }

        byte[] result = new byte[len];
        System.arraycopy(T, 0, result, 0, len);
        return result;
    }

    




    public static Key makeHMACKey(byte[] key) {
        if (key.length == 0) {
            key = new byte[BLOCKSIZE];
        }
        return new SecretKeySpec(key, "HmacSHA256");
    }

    




    public static Mac makeHMACHasher(byte[] key) {
        Mac hmacHasher = null;
        try {
            hmacHasher = Mac.getInstance("hmacSHA256");
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }

        try {
            hmacHasher.init(makeHMACKey(key));
        } catch (InvalidKeyException e) {
            e.printStackTrace();
        }

        return hmacHasher;
    }

    




    public static byte[] digestBytes(byte[] message, Mac hasher) {
        hasher.update(message);
        byte[] ret = hasher.doFinal();
        hasher.reset();
        return ret;
    }
}
