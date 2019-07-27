


























package ch.boye.httpclientandroidlib.client.config;

import ch.boye.httpclientandroidlib.annotation.Immutable;






@Immutable
public final class AuthSchemes {

    



    public static final String BASIC = "Basic";

    


    public static final String DIGEST = "Digest";

    




    public static final String NTLM = "NTLM";

    


    public static final String SPNEGO = "negotiate";

    


    public static final String KERBEROS = "Kerberos";

    private AuthSchemes() {
    }

}
