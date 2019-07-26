


























package ch.boye.httpclientandroidlib.impl;

import ch.boye.httpclientandroidlib.ConnectionReuseStrategy;
import ch.boye.httpclientandroidlib.HttpConnection;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpEntity;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.protocol.HttpContext;
import ch.boye.httpclientandroidlib.protocol.ExecutionContext;
import ch.boye.httpclientandroidlib.TokenIterator;
import ch.boye.httpclientandroidlib.message.BasicTokenIterator;



















public class DefaultConnectionReuseStrategy implements ConnectionReuseStrategy {

    public DefaultConnectionReuseStrategy() {
        super();
    }

    
    public boolean keepAlive(final HttpResponse response,
                             final HttpContext context) {
        if (response == null) {
            throw new IllegalArgumentException
                ("HTTP response may not be null.");
        }
        if (context == null) {
            throw new IllegalArgumentException
                ("HTTP context may not be null.");
        }

        HttpConnection conn = (HttpConnection)
            context.getAttribute(ExecutionContext.HTTP_CONNECTION);

        if (conn != null && !conn.isOpen())
            return false;
        

        
        
        HttpEntity entity = response.getEntity();
        ProtocolVersion ver = response.getStatusLine().getProtocolVersion();
        if (entity != null) {
            if (entity.getContentLength() < 0) {
                if (!entity.isChunked() ||
                    ver.lessEquals(HttpVersion.HTTP_1_0)) {
                    
                    
                    return false;
                }
            }
        }

        
        
        
        HeaderIterator hit = response.headerIterator(HTTP.CONN_DIRECTIVE);
        if (!hit.hasNext())
            hit = response.headerIterator("Proxy-Connection");

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (hit.hasNext()) {
            try {
                TokenIterator ti = createTokenIterator(hit);
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
                    return true;
                

            } catch (ParseException px) {
                
                
                return false;
            }
        }

        
        return !ver.lessEquals(HttpVersion.HTTP_1_0);
    }


    








    protected TokenIterator createTokenIterator(HeaderIterator hit) {
        return new BasicTokenIterator(hit);
    }
}
