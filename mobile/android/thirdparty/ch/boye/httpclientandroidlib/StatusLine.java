


























package ch.boye.httpclientandroidlib;















public interface StatusLine {

    ProtocolVersion getProtocolVersion();

    int getStatusCode();

    String getReasonPhrase();

}
