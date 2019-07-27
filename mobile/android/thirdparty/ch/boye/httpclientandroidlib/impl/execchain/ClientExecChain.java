


























package ch.boye.httpclientandroidlib.impl.execchain;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.client.methods.CloseableHttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpExecutionAware;
import ch.boye.httpclientandroidlib.client.methods.HttpRequestWrapper;
import ch.boye.httpclientandroidlib.client.protocol.HttpClientContext;
import ch.boye.httpclientandroidlib.conn.routing.HttpRoute;













public interface ClientExecChain {

    














    CloseableHttpResponse execute(
            HttpRoute route,
            HttpRequestWrapper request,
            HttpClientContext clientContext,
            HttpExecutionAware execAware) throws IOException, HttpException;

}
