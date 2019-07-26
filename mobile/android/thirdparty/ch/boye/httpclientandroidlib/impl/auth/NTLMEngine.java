

























package ch.boye.httpclientandroidlib.impl.auth;








public interface NTLMEngine {

    








    String generateType1Msg(
            String domain,
            String workstation) throws NTLMEngineException;

    











    String generateType3Msg(
            String username,
            String password,
            String domain,
            String workstation,
            String challenge) throws NTLMEngineException;

}
