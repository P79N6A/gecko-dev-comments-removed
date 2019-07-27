


























package ch.boye.httpclientandroidlib.message;

import java.util.Locale;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.ReasonPhraseCatalog;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;








@NotThreadSafe
public class BasicHttpResponse extends AbstractHttpMessage implements HttpResponse {

    private StatusLine          statusline;
    private ProtocolVersion     ver;
    private int                 code;
    private String              reasonPhrase;
    private HttpEntity          entity;
    private final ReasonPhraseCatalog reasonCatalog;
    private Locale              locale;

    










    public BasicHttpResponse(final StatusLine statusline,
                             final ReasonPhraseCatalog catalog,
                             final Locale locale) {
        super();
        this.statusline = Args.notNull(statusline, "Status line");
        this.ver = statusline.getProtocolVersion();
        this.code = statusline.getStatusCode();
        this.reasonPhrase = statusline.getReasonPhrase();
        this.reasonCatalog = catalog;
        this.locale = locale;
    }

    






    public BasicHttpResponse(final StatusLine statusline) {
        super();
        this.statusline = Args.notNull(statusline, "Status line");
        this.ver = statusline.getProtocolVersion();
        this.code = statusline.getStatusCode();
        this.reasonPhrase = statusline.getReasonPhrase();
        this.reasonCatalog = null;
        this.locale = null;
    }

    









    public BasicHttpResponse(final ProtocolVersion ver,
                             final int code,
                             final String reason) {
        super();
        Args.notNegative(code, "Status code");
        this.statusline = null;
        this.ver = ver;
        this.code = code;
        this.reasonPhrase = reason;
        this.reasonCatalog = null;
        this.locale = null;
    }


    
    public ProtocolVersion getProtocolVersion() {
        return this.ver;
    }

    
    public StatusLine getStatusLine() {
        if (this.statusline == null) {
            this.statusline = new BasicStatusLine(
                    this.ver != null ? this.ver : HttpVersion.HTTP_1_1,
                    this.code,
                    this.reasonPhrase != null ? this.reasonPhrase : getReason(this.code));
        }
        return this.statusline;
    }

    
    public HttpEntity getEntity() {
        return this.entity;
    }

    public Locale getLocale() {
        return this.locale;
    }

    
    public void setStatusLine(final StatusLine statusline) {
        this.statusline = Args.notNull(statusline, "Status line");
        this.ver = statusline.getProtocolVersion();
        this.code = statusline.getStatusCode();
        this.reasonPhrase = statusline.getReasonPhrase();
    }

    
    public void setStatusLine(final ProtocolVersion ver, final int code) {
        Args.notNegative(code, "Status code");
        this.statusline = null;
        this.ver = ver;
        this.code = code;
        this.reasonPhrase = null;
    }

    
    public void setStatusLine(
            final ProtocolVersion ver, final int code, final String reason) {
        Args.notNegative(code, "Status code");
        this.statusline = null;
        this.ver = ver;
        this.code = code;
        this.reasonPhrase = reason;
    }

    
    public void setStatusCode(final int code) {
        Args.notNegative(code, "Status code");
        this.statusline = null;
        this.code = code;
        this.reasonPhrase = null;
    }

    
    public void setReasonPhrase(final String reason) {
        this.statusline = null;
        this.reasonPhrase = reason;
    }

    
    public void setEntity(final HttpEntity entity) {
        this.entity = entity;
    }

    public void setLocale(final Locale locale) {
        this.locale =  Args.notNull(locale, "Locale");
        this.statusline = null;
    }

    








    protected String getReason(final int code) {
        return this.reasonCatalog != null ? this.reasonCatalog.getReason(code,
                this.locale != null ? this.locale : Locale.getDefault()) : null;
    }

    @Override
    public String toString() {
        return getStatusLine() + " " + this.headergroup;
    }

}
