



package org.mozilla.gecko.fxa.login;

import org.mozilla.gecko.browserid.BrowserIDKeyPair;
import org.mozilla.gecko.sync.ExtendedJSONObject;
import org.mozilla.gecko.sync.Utils;

public abstract class TokensAndKeysState extends State {
  protected final byte[] sessionToken;
  protected final byte[] kA;
  protected final byte[] kB;
  protected final BrowserIDKeyPair keyPair;

  public TokensAndKeysState(StateLabel stateLabel, String email, String uid, byte[] sessionToken, byte[] kA, byte[] kB, BrowserIDKeyPair keyPair) {
    super(stateLabel, email, uid, true);
    Utils.throwIfNull(sessionToken, kA, kB, keyPair);
    this.sessionToken = sessionToken;
    this.kA = kA;
    this.kB = kB;
    this.keyPair = keyPair;
  }

  @Override
  public ExtendedJSONObject toJSONObject() {
    ExtendedJSONObject o = super.toJSONObject();
    
    o.put("sessionToken", Utils.byte2Hex(sessionToken));
    o.put("kA", Utils.byte2Hex(kA));
    o.put("kB", Utils.byte2Hex(kB));
    o.put("keyPair", keyPair.toJSONObject());
    return o;
  }

  @Override
  public Action getNeededAction() {
    return Action.None;
  }
}
