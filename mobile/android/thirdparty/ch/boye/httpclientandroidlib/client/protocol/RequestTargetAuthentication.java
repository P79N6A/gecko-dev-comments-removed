


























package ch.boye.httpclientandroidlib.client.protocol;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.auth.AUTH;
import ch.boye.httpclientandroidlib.auth.AuthState;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;









@Deprecated
@Immutable
public class RequestTargetAuthentication extends RequestAuthenticationBase {

    public RequestTargetAuthentication() {
        super();
    }

    public void process(final HttpRequest request, final HttpContext context)
            throws HttpException, IOException {
        Args.notNull(request, "HTTP request");
        Args.notNull(context, "HTTP context");

        final String method = request.getRequestLine().getMethod();
        if (method.equalsIgnoreCase("CONNECT")) {
            return;
        }

        if (request.containsHeader(AUTH.WWW_AUTH_RESP)) {
            return;
        }

        
        final AuthState authState = (AuthState) context.getAttribute(
                ClientContext.TARGET_AUTH_STATE);
        if (authState == null) {
            this.log.debug("Target auth state not set in the context");
            return;
        }
        if (this.log.isDebugEnabled()) {
            this.log.debug("Target auth state: " + authState.getState());
        }
        process(authState, request, context);
    }

}
