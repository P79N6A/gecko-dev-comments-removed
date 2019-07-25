




































package org.mozilla.gecko.sync.jpake;

import java.math.BigInteger;
import java.security.SecureRandom;




public class JPakeNumGeneratorRandom implements JPakeNumGenerator {

  @Override
  public BigInteger generateFromRange(BigInteger r) {
    int maxBytes = (int) Math.ceil(((double) r.bitLength()) / 8);

    byte[] bytes = new byte[maxBytes];
    new SecureRandom().nextBytes(bytes);
    BigInteger randInt = new BigInteger(bytes);
    
    
    return randInt.mod(r);
  }

}
