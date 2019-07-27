


























package ch.boye.httpclientandroidlib.client;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.protocol.HttpContext;












@Deprecated
public interface RequestDirector {


    




















    HttpResponse execute(HttpHost target, HttpRequest request, HttpContext context)
        throws HttpException, IOException;

}
