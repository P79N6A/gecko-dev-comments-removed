


























package ch.boye.httpclientandroidlib;












public interface RequestLine {

    String getMethod();

    ProtocolVersion getProtocolVersion();

    String getUri();

}
