

























package ch.boye.httpclientandroidlib.impl.auth;

import java.io.IOException;












@Deprecated
public interface SpnegoTokenGenerator {

    byte [] generateSpnegoDERObject(byte [] kerberosTicket) throws IOException;

}
