




































package org.mozilla.gecko.sync.jpake;

import java.math.BigInteger;

public class JPakeParty {

  
  public String       signerId;
  public BigInteger   gx1;
  public Zkp          zkp1;

  public BigInteger   x2;
  public BigInteger   gx2;
  public Zkp          zkp2;

  public BigInteger   thisA;
  public Zkp          thisZkpA;

  
  public BigInteger   gx3;
  public Zkp          zkp3;

  public BigInteger   gx4;
  public Zkp          zkp4;

  public BigInteger   otherA;
  public Zkp          otherZkpA;


  public JPakeParty(String mySignerId) {
    this.signerId = mySignerId;
  }
}
