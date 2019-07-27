


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseInterceptor;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class ResponseServer implements HttpResponseInterceptor {

    private final String originServer;

    


    public ResponseServer(final String originServer) {
        super();
        this.originServer = originServer;
    }

    public ResponseServer() {
        this(null);
    }

    public void process(final HttpResponse response, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(response, "HTTP response");
        if (!response.containsHeader(HTTP.SERVER_HEADER)) {
            if (this.originServer != null) {
                response.addHeader(HTTP.SERVER_HEADER, this.originServer);
            }
        }
    }

}
