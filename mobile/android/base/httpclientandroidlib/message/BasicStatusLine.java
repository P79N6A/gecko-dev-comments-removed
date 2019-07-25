


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.StatusLine;








public class BasicStatusLine implements StatusLine, Cloneable, Serializable {

    private static final long serialVersionUID = -2443303766890459269L;

    

    
    private final ProtocolVersion protoVersion;

    
    private final int statusCode;

    
    private final String reasonPhrase;

    
    







    public BasicStatusLine(final ProtocolVersion version, int statusCode,
                           final String reasonPhrase) {
        super();
        if (version == null) {
            throw new IllegalArgumentException
                ("Protocol version may not be null.");
        }
        if (statusCode < 0) {
            throw new IllegalArgumentException
                ("Status code may not be negative.");
        }
        this.protoVersion = version;
        this.statusCode   = statusCode;
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

    public String toString() {
        
        return BasicLineFormatter.DEFAULT
            .formatStatusLine(null, this).toString();
    }

    public Object clone() throws CloneNotSupportedException {
        return super.clone();
    }

}
