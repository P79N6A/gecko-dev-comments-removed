


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class BasicStatusLine implements StatusLine, Cloneable, Serializable {

    private static final long serialVersionUID = -2443303766890459269L;

    

    
    private final ProtocolVersion protoVersion;

    
    private final int statusCode;

    
    private final String reasonPhrase;

    
    







    public BasicStatusLine(final ProtocolVersion version, final int statusCode,
                           final String reasonPhrase) {
        super();
        this.protoVersion = Args.notNull(version, "Version");
        this.statusCode = Args.notNegative(statusCode, "Status code");
        this.reasonPhrase = reasonPhrase;
    }

    

    public int getStatusCode() {
        return this.statusCode;
    }

    public ProtocolVersion getProtocolVersion() {
        return this.protoVersion;
    }

    public String getReasonPhrase() {
        return this.reasonPhrase;
    }

    @Override
    public String toString() {
        
        return BasicLineFormatter.INSTANCE.formatStatusLine(null, this).toString();
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
