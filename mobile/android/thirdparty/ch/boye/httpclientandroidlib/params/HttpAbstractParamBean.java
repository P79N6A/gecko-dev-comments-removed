


























package ch.boye.httpclientandroidlib.params;

import ch.boye.httpclientandroidlib.util.Args;







@Deprecated
public abstract class HttpAbstractParamBean {

    protected final HttpParams params;

    public HttpAbstractParamBean (final HttpParams params) {
        super();
        this.params = Args.notNull(params, "HTTP parameters");
    }

}
