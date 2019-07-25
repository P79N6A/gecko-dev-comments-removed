


























package ch.boye.httpclientandroidlib.client;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.client.methods.HttpUriRequest;
import ch.boye.httpclientandroidlib.conn.ClientConnectionManager;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.protocol.HttpContext;


































































public interface HttpClient {


    







    HttpParams getParams();

    




    ClientConnectionManager getConnectionManager();

    












    HttpResponse execute(HttpUriRequest request)
        throws IOException, ClientProtocolException;

    















    HttpResponse execute(HttpUriRequest request, HttpContext context)
        throws IOException, ClientProtocolException;

    
















    HttpResponse execute(HttpHost target, HttpRequest request)
        throws IOException, ClientProtocolException;

    


















    HttpResponse execute(HttpHost target, HttpRequest request,
                         HttpContext context)
        throws IOException, ClientProtocolException;

    










    <T> T execute(
            HttpUriRequest request,
            ResponseHandler<? extends T> responseHandler)
        throws IOException, ClientProtocolException;

    










    <T> T execute(
            HttpUriRequest request,
            ResponseHandler<? extends T> responseHandler,
            HttpContext context)
        throws IOException, ClientProtocolException;

    














    <T> T execute(
            HttpHost target,
            HttpRequest request,
            ResponseHandler<? extends T> responseHandler)
        throws IOException, ClientProtocolException;

    
















    <T> T execute(
            HttpHost target,
            HttpRequest request,
            ResponseHandler<? extends T> responseHandler,
            HttpContext context)
        throws IOException, ClientProtocolException;

}
