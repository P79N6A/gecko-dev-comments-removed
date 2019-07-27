


























package ch.boye.httpclientandroidlib;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.annotation.Immutable;














@Immutable
public final class HttpVersion extends ProtocolVersion
    implements Serializable {

    private static final long serialVersionUID = -5856653513894415344L;

    
    public static final String HTTP = "HTTP";

    
    public static final HttpVersion HTTP_0_9 = new HttpVersion(0, 9);

    
    public static final HttpVersion HTTP_1_0 = new HttpVersion(1, 0);

    
    public static final HttpVersion HTTP_1_1 = new HttpVersion(1, 1);


    







    public HttpVersion(final int major, final int minor) {
        super(HTTP, major, minor);
    }


    







    @Override
    public ProtocolVersion forVersion(final int major, final int minor) {

        if ((major == this.major) && (minor == this.minor)) {
            return this;
        }

        if (major == 1) {
            if (minor == 0) {
                return HTTP_1_0;
            }
            if (minor == 1) {
                return HTTP_1_1;
            }
        }
        if ((major == 0) && (minor == 9)) {
            return HTTP_0_9;
        }

        
        return new HttpVersion(major, minor);
    }

}
