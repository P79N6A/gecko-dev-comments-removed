


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpEntityEnclosingRequest;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.params.CoreProtocolPNames;
import ch.boye.httpclientandroidlib.util.Args;








@Immutable
@SuppressWarnings("deprecation")
public class RequestExpectContinue implements HttpRequestInterceptor {

    private final boolean activeByDefault;

    


    @Deprecated
    public RequestExpectContinue() {
        this(false);
    }

    


    public RequestExpectContinue(final boolean activeByDefault) {
        super();
        this.activeByDefault = activeByDefault;
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");

        if (!request.containsHeader(HTTP.EXPECT_DIRECTIVE)) {
            if (request instanceof HttpEntityEnclosingRequest) {
                final ProtocolVersion ver = request.getRequestLine().getProtocolVersion();
                final HttpEntity entity = ((HttpEntityEnclosingRequest)request).getEntity();
                
                if (entity != null
                        && entity.getContentLength() != 0 && !ver.lessEquals(HttpVersion.HTTP_1_0)) {
                    final boolean active = request.getParams().getBooleanParameter(
                            CoreProtocolPNames.USE_EXPECT_CONTINUE, this.activeByDefault);
                    if (active) {
                        request.addHeader(HTTP.EXPECT_DIRECTIVE, HTTP.EXPECT_CONTINUE);
                    }
                }
            }
        }
    }

}
