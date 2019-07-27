


























package ch.boye.httpclientandroidlib.impl;

import java.util.Locale;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.ReasonPhraseCatalog;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.message.BasicHttpResponse;
import ch.boye.httpclientandroidlib.message.BasicStatusLine;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;






@Immutable
public class DefaultHttpResponseFactory implements HttpResponseFactory {

    public static final DefaultHttpResponseFactory INSTANCE = new DefaultHttpResponseFactory();

    
    protected final ReasonPhraseCatalog reasonCatalog;


    




    public DefaultHttpResponseFactory(final ReasonPhraseCatalog catalog) {
        this.reasonCatalog = Args.notNull(catalog, "Reason phrase catalog");
    }

    



    public DefaultHttpResponseFactory() {
        this(EnglishReasonPhraseCatalog.INSTANCE);
    }


    
    public HttpResponse newHttpResponse(
            final ProtocolVersion ver,
            final int status,
            final HttpContext context) {
        Args.notNull(ver, "HTTP version");
        final Locale loc = determineLocale(context);
        final String reason   = this.reasonCatalog.getReason(status, loc);
        final StatusLine statusline = new BasicStatusLine(ver, status, reason);
        return new BasicHttpResponse(statusline, this.reasonCatalog, loc);
    }


    
    public HttpResponse newHttpResponse(
            final StatusLine statusline,
            final HttpContext context) {
        Args.notNull(statusline, "Status line");
        return new BasicHttpResponse(statusline, this.reasonCatalog, determineLocale(context));
    }

    








    protected Locale determineLocale(final HttpContext context) {
        return Locale.getDefault();
    }

}
