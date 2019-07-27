


























package ch.boye.httpclientandroidlib.protocol;

import java.util.Map;

import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.util.Args;



















@ThreadSafe 
@Deprecated
public class HttpRequestHandlerRegistry implements HttpRequestHandlerResolver {

    private final UriPatternMatcher<HttpRequestHandler> matcher;

    public HttpRequestHandlerRegistry() {
        matcher = new UriPatternMatcher<HttpRequestHandler>();
    }

    






    public void register(final String pattern, final HttpRequestHandler handler) {
        Args.notNull(pattern, "URI request pattern");
        Args.notNull(handler, "Request handler");
        matcher.register(pattern, handler);
    }

    




    public void unregister(final String pattern) {
        matcher.unregister(pattern);
    }

    



    public void setHandlers(final Map<String, HttpRequestHandler> map) {
        matcher.setObjects(map);
    }

    





    public Map<String, HttpRequestHandler> getHandlers() {
        return matcher.getObjects();
    }

    public HttpRequestHandler lookup(final String requestURI) {
        return matcher.lookup(requestURI);
    }

}
