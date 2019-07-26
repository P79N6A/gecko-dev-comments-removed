

























package ch.boye.httpclientandroidlib.auth;

import java.security.Principal;








public interface Credentials {

    Principal getUserPrincipal();

    String getPassword();

}
