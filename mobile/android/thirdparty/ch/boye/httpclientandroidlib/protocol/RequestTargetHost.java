


























package ch.boye.httpclientandroidlib.protocol;

import java.io.IOException;
import java.net.InetAddress;

import ch.boye.httpclientandroidlib.HttpConnection;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpHost;
import ch.boye.httpclientandroidlib.HttpInetConnection;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.HttpRequestInterceptor;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.util.Args;







@Immutable
public class RequestTargetHost implements HttpRequestInterceptor {

    public RequestTargetHost() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");

        final HttpCoreContext corecontext = HttpCoreContext.adapt(context);

        final ProtocolVersion ver = request.getRequestLine().getProtocolVersion();
        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT") && ver.lessEquals(HttpVersion.HTTP_1_0)) {
            return;
        }

        if (!request.containsHeader(HTTP.TARGET_HOST)) {
            HttpHost targethost = corecontext.getTargetHost();
            if (targethost == null) {
                final HttpConnection conn = corecontext.getConnection();
                if (conn instanceof HttpInetConnection) {
                    
                    
                    final InetAddress address = ((HttpInetConnection) conn).getRemoteAddress();
                    final int port = ((HttpInetConnection) conn).getRemotePort();
                    if (address != null) {
                        targethost = new HttpHost(address.getHostName(), port);
                    }
                }
                if (targethost == null) {
                    if (ver.lessEquals(HttpVersion.HTTP_1_0)) {
                        return;
                    } else {
                        throw new ProtocolException("Target host missing");
                    }
                }
            }
            request.addHeader(HTTP.TARGET_HOST, targethost.toHostString());
        }
    }

}
