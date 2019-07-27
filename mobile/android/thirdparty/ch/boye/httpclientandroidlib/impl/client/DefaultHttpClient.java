


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.client.protocol.RequestAddCookies;
import ch.boye.httpclientandroidlib.client.protocol.RequestAuthCache;
import ch.boye.httpclientandroidlib.client.protocol.RequestClientConnControl;
import ch.boye.httpclientandroidlib.client.protocol.RequestDefaultHeaders;
import ch.boye.httpclientandroidlib.client.protocol.RequestProxyAuthentication;
import ch.boye.httpclientandroidlib.client.protocol.RequestTargetAuthentication;
import ch.boye.httpclientandroidlib.client.protocol.ResponseProcessCookies;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.params.HttpConnectionParams;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.params.HttpProtocolParams;
import ch.boye.httpclientandroidlib.params.SyncBasicHttpParams;
import ch.boye.httpclientandroidlib.protocol.BasicHttpProcessor;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.RequestContent;
import ch.boye.httpclientandroidlib.protocol.RequestExpectContinue;
import ch.boye.httpclientandroidlib.protocol.RequestTargetHost;
import ch.boye.httpclientandroidlib.protocol.RequestUserAgent;
































































@ThreadSafe
@Deprecated
public class DefaultHttpClient extends AbstractHttpClient {

    





    public DefaultHttpClient(
            final ClientConnectionManager conman,
            final HttpParams params) {
        super(conman, params);
    }


    


    public DefaultHttpClient(
            final ClientConnectionManager conman) {
        super(conman, null);
    }


    public DefaultHttpClient(final HttpParams params) {
        super(null, params);
    }


    public DefaultHttpClient() {
        super(null, null);
    }


    




    @Override
    protected HttpParams createHttpParams() {
        final HttpParams params = new SyncBasicHttpParams();
        setDefaultHttpParams(params);
        return params;
    }

    















    public static void setDefaultHttpParams(final HttpParams params) {
        HttpProtocolParams.setVersion(params, HttpVersion.HTTP_1_1);
        HttpProtocolParams.setContentCharset(params, HTTP.DEF_CONTENT_CHARSET.name());
        HttpConnectionParams.setTcpNoDelay(params, true);
        HttpConnectionParams.setSocketBufferSize(params, 8192);
        HttpProtocolParams.setUserAgent(params, HttpClientBuilder.DEFAULT_USER_AGENT);
    }

    

















    @Override
    protected BasicHttpProcessor createHttpProcessor() {
        final BasicHttpProcessor httpproc = new BasicHttpProcessor();
        httpproc.addInterceptor(new RequestDefaultHeaders());
        
        httpproc.addInterceptor(new RequestContent());
        httpproc.addInterceptor(new RequestTargetHost());
        
        httpproc.addInterceptor(new RequestClientConnControl());
        httpproc.addInterceptor(new RequestUserAgent());
        httpproc.addInterceptor(new RequestExpectContinue());
        
        httpproc.addInterceptor(new RequestAddCookies());
        httpproc.addInterceptor(new ResponseProcessCookies());
        
        httpproc.addInterceptor(new RequestAuthCache());
        httpproc.addInterceptor(new RequestTargetAuthentication());
        httpproc.addInterceptor(new RequestProxyAuthentication());
        return httpproc;
    }

}
