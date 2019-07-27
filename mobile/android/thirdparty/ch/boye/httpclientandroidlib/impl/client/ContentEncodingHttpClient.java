

























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.protocol.RequestAcceptEncoding;
import ch.boye.httpclientandroidlib.client.protocol.ResponseContentEncoding;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.BasicHttpProcessor;

















@Deprecated
@ThreadSafe 
public class ContentEncodingHttpClient extends DefaultHttpClient {

    





    public ContentEncodingHttpClient(final ClientConnectionManager conman, final HttpParams params) {
        super(conman, params);
    }

    


    public ContentEncodingHttpClient(final HttpParams params) {
        this(null, params);
    }

    


    public ContentEncodingHttpClient() {
        this(null);
    }

    


    @Override
    protected BasicHttpProcessor createHttpProcessor() {
        final BasicHttpProcessor result = super.createHttpProcessor();

        result.addRequestInterceptor(new RequestAcceptEncoding());
        result.addResponseInterceptor(new ResponseContentEncoding());

        return result;
    }

}
