




































package org.mozilla.gecko.sync.jpake;

import java.math.BigInteger;

import org.mozilla.gecko.sync.Utils;




public class JPakeNumGeneratorRandom implements JPakeNumGenerator {

  @Override
  public BigInteger generateFromRange(BigInteger r) {
    return Utils.generateBigIntegerLessThan(r);
  }
}
