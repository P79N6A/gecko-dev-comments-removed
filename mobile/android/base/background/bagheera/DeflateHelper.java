



package org.mozilla.gecko.background.bagheera;

import java.io.UnsupportedEncodingException;
import java.util.zip.Deflater;

import ch.boye.httpclientandroidlib.HttpEntity;

public class DeflateHelper {
  









  public static int deflateBound(final int sourceLen) {
    return sourceLen + ((sourceLen + 7) >> 3) + ((sourceLen + 63) >> 6) + 5 + 6;
  }

  



  public static int deflate(byte[] input, byte[] output) {
    final Deflater deflater = new Deflater();
    deflater.setInput(input);
    deflater.finish();

    final int length = deflater.deflate(output);
    deflater.end();
    return length;
  }

  















  @SuppressWarnings("javadoc")
  public static HttpEntity deflateBytes(final byte[] bytes) {
    
    
    

    final byte[] out = new byte[deflateBound(bytes.length)];
    final int outLength = deflate(bytes, out);
    return new BoundedByteArrayEntity(out, 0, outLength);
  }

  public static HttpEntity deflateBody(final String payload) {
    final byte[] bytes;
    try {
      bytes = payload.getBytes("UTF-8");
    } catch (UnsupportedEncodingException ex) {
      
      throw new RuntimeException(ex);
    }
    return deflateBytes(bytes);
  }
}