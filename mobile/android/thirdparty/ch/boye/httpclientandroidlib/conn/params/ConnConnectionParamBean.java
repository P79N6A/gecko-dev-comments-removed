


























package ch.boye.httpclientandroidlib.conn.params;

import ch.boye.httpclientandroidlib.params.HttpAbstractParamBean;
import ch.boye.httpclientandroidlib.params.HttpParams;











@Deprecated
public class ConnConnectionParamBean extends HttpAbstractParamBean {

    public ConnConnectionParamBean (final HttpParams params) {
        super(params);
    }

    



    @Deprecated
    public void setMaxStatusLineGarbage (final int maxStatusLineGarbage) {
        params.setIntParameter(ConnConnectionPNames.MAX_STATUS_LINE_GARBAGE, maxStatusLineGarbage);
    }

}
