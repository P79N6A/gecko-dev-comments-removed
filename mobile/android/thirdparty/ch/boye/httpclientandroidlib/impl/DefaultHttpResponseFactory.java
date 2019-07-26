


























package ch.boye.httpclientandroidlib.impl;

import java.util.Locale;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.message.BasicHttpResponse;
import ch.boye.httpclientandroidlib.message.BasicStatusLine;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.ReasonPhraseCatalog;
import ch.boye.httpclientandroidlib.impl.EnglishReasonPhraseCatalog;






public class DefaultHttpResponseFactory implements HttpResponseFactory {

    
    protected final ReasonPhraseCatalog reasonCatalog;


    




    public DefaultHttpResponseFactory(ReasonPhraseCatalog catalog) {
        if (catalog == null) {
            throw new IllegalArgumentException
                ("Reason phrase catalog must not be null.");
        }
        this.reasonCatalog = catalog;
    }

    



    public DefaultHttpResponseFactory() {
        this(EnglishReasonPhraseCatalog.INSTANCE);
    }


    
    public HttpResponse newHttpResponse(final ProtocolVersion ver,
                                        final int status,
                                        HttpContext context) {
        if (ver == null) {
            throw new IllegalArgumentException("HTTP version may not be null");
        }
        final Locale loc      = determineLocale(context);
        final String reason   = reasonCatalog.getReason(status, loc);
        StatusLine statusline = new BasicStatusLine(ver, status, reason);
        return new BasicHttpResponse(statusline, reasonCatalog, loc);
    }


    
    public HttpResponse newHttpResponse(final StatusLine statusline,
                                        HttpContext context) {
        if (statusline == null) {
            throw new IllegalArgumentException("Status line may not be null");
        }
        final Locale loc = determineLocale(context);
        return new BasicHttpResponse(statusline, reasonCatalog, loc);
    }


    








    protected Locale determineLocale(HttpContext context) {
        return Locale.getDefault();
    }
}
