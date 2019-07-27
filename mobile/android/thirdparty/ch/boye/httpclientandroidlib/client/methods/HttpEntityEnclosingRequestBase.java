


























package ch.boye.httpclientandroidlib.client.methods;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.client.utils.CloneUtils;
import ch.boye.httpclientandroidlib.protocol.HTTP;







@NotThreadSafe 
public abstract class HttpEntityEnclosingRequestBase
    extends HttpRequestBase implements HttpEntityEnclosingRequest {

    private HttpEntity entity;

    public HttpEntityEnclosingRequestBase() {
        super();
    }

    public HttpEntity getEntity() {
        return this.entity;
    }

    public void setEntity(final HttpEntity entity) {
        this.entity = entity;
    }

    public boolean expectContinue() {
        final Header expect = getFirstHeader(HTTP.EXPECT_DIRECTIVE);
        return expect != null && HTTP.EXPECT_CONTINUE.equalsIgnoreCase(expect.getValue());
    }

    @Override
    public Object clone() throws CloneNotSupportedException {
        final HttpEntityEnclosingRequestBase clone =
            (HttpEntityEnclosingRequestBase) super.clone();
        if (this.entity != null) {
            clone.entity = CloneUtils.cloneObject(this.entity);
        }
        return clone;
    }

}
