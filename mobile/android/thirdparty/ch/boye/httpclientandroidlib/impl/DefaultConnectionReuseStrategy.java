


























package ch.boye.httpclientandroidlib.impl;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpStatus;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.TokenIterator;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.message.BasicTokenIterator;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.util.Args;



















@Immutable
public class DefaultConnectionReuseStrategy implements ConnectionReuseStrategy {

    public static final DefaultConnectionReuseStrategy INSTANCE = new DefaultConnectionReuseStrategy();

    public DefaultConnectionReuseStrategy() {
        super();
    }

    
    public boolean keepAlive(final HttpResponse response,
                             final HttpContext context) {
        Args.notNull(response, "HTTP response");
        Args.notNull(context, "HTTP context");

        
        
        final ProtocolVersion ver = response.getStatusLine().getProtocolVersion();
        final Header teh = response.getFirstHeader(HTTP.TRANSFER_ENCODING);
        if (teh != null) {
            if (!HTTP.CHUNK_CODING.equalsIgnoreCase(teh.getValue())) {
                return false;
            }
        } else {
            if (canResponseHaveBody(response)) {
                final Header[] clhs = response.getHeaders(HTTP.CONTENT_LEN);
                
                if (clhs.length == 1) {
                    final Header clh = clhs[0];
                    try {
                        final int contentLen = Integer.parseInt(clh.getValue());
                        if (contentLen < 0) {
                            return false;
                        }
                    } catch (final NumberFormatException ex) {
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }

        
        
        
        HeaderIterator hit = response.headerIterator(HTTP.CONN_DIRECTIVE);
        if (!hit.hasNext()) {
            hit = response.headerIterator("Proxy-Connection");
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (hit.hasNext()) {
            try {
                final TokenIterator ti = createTokenIterator(hit);
                boolean keepalive = false;
                while (ti.hasNext()) {
                    final String token = ti.nextToken();
                    if (HTTP.CONN_CLOSE.equalsIgnoreCase(token)) {
                        return false;
                    } else if (HTTP.CONN_KEEP_ALIVE.equalsIgnoreCase(token)) {
                        
                        keepalive = true;
                    }
                }
                if (keepalive)
                 {
                    return true;
                
                }

            } catch (final ParseException px) {
                
                
                return false;
            }
        }

        
        return !ver.lessEquals(HttpVersion.HTTP_1_0);
    }


    








    protected TokenIterator createTokenIterator(final HeaderIterator hit) {
        return new BasicTokenIterator(hit);
    }

    private boolean canResponseHaveBody(final HttpResponse response) {
        final int status = response.getStatusLine().getStatusCode();
        return status >= HttpStatus.SC_OK
            && status != HttpStatus.SC_NO_CONTENT
            && status != HttpStatus.SC_NOT_MODIFIED
            && status != HttpStatus.SC_RESET_CONTENT;
    }

}
