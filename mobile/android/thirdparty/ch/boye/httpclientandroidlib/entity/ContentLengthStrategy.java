


























package ch.boye.httpclientandroidlib.entity;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;







public interface ContentLengthStrategy {

    public static final int IDENTITY         = -1;
    public static final int CHUNKED          = -2;

    










    long determineLength(HttpMessage message) throws HttpException;

}
