


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class BasicRequestLine implements RequestLine, Cloneable, Serializable {

    private static final long serialVersionUID = 2810581718468737193L;

    private final ProtocolVersion protoversion;
    private final String method;
    private final String uri;

    public BasicRequestLine(final String method,
                            final String uri,
                            final ProtocolVersion version) {
        super();
        this.method = Args.notNull(method, "Method");
        this.uri = Args.notNull(uri, "URI");
        this.protoversion = Args.notNull(version, "Version");
    }

    public String getMethod() {
        return this.method;
    }

    public ProtocolVersion getProtocolVersion() {
        return this.protoversion;
    }

    public String getUri() {
        return this.uri;
    }

    @Override
    public String toString() {
        
        return BasicLineFormatter.INSTANCE.formatRequestLine(null, this).toString();
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
