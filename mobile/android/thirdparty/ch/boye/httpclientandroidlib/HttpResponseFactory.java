


























package ch.boye.httpclientandroidlib;

import ch.boye.httpclientandroidlib.protocol.HttpContext;






public interface HttpResponseFactory {

    










    HttpResponse newHttpResponse(ProtocolVersion ver, int status,
                                 HttpContext context);

    










    HttpResponse newHttpResponse(StatusLine statusline,
                                 HttpContext context);

}
