


























package ch.boye.httpclientandroidlib.impl.client;

import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.client.methods.HttpGet;
import ch.boye.httpclientandroidlib.client.methods.HttpHead;
import ch.boye.httpclientandroidlib.client.methods.HttpPost;









@Immutable
public class LaxRedirectStrategy extends DefaultRedirectStrategy {

    


    private static final String[] REDIRECT_METHODS = new String[] {
        HttpGet.METHOD_NAME,
        HttpPost.METHOD_NAME,
        HttpHead.METHOD_NAME
    };

    @Override
    protected boolean isRedirectable(final String method) {
        for (final String m: REDIRECT_METHODS) {
            if (m.equalsIgnoreCase(method)) {
                return true;
            }
        }
        return false;
    }

}
