


























package ch.boye.httpclientandroidlib.client.params;

import ch.boye.httpclientandroidlib.annotation.Immutable;








@Deprecated
@Immutable
public final class AuthPolicy {

    private AuthPolicy() {
        super();
    }

    




    public static final String NTLM = "NTLM";

    


    public static final String DIGEST = "Digest";

    



    public static final String BASIC = "Basic";

    




    public static final String SPNEGO = "negotiate";

    




    public static final String KERBEROS = "Kerberos";

}
