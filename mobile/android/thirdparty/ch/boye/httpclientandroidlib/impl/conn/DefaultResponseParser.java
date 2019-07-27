


























package ch.boye.httpclientandroidlib.impl.conn;

import java.io.IOException;

import ch.boye.httpclientandroidlib.androidextra.HttpClientAndroidLog;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.NoHttpResponseException;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.ThreadSafe;
import ch.boye.httpclientandroidlib.impl.io.AbstractMessageParser;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.LineParser;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
















@Deprecated
@ThreadSafe 
public class DefaultResponseParser extends AbstractMessageParser<HttpMessage> {

    public HttpClientAndroidLog log = new HttpClientAndroidLog(getClass());

    private final HttpResponseFactory responseFactory;
    private final CharArrayBuffer lineBuf;
    private final int maxGarbageLines;

    public DefaultResponseParser(
            final SessionInputBuffer buffer,
            final LineParser parser,
            final HttpResponseFactory responseFactory,
            final HttpParams params) {
        super(buffer, parser, params);
        Args.notNull(responseFactory, "Response factory");
        this.responseFactory = responseFactory;
        this.lineBuf = new CharArrayBuffer(128);
        this.maxGarbageLines = getMaxGarbageLines(params);
    }

    protected int getMaxGarbageLines(final HttpParams params) {
        return params.getIntParameter(
                ch.boye.httpclientandroidlib.conn.params.ConnConnectionPNames.MAX_STATUS_LINE_GARBAGE,
                Integer.MAX_VALUE);
    }

    @Override
    protected HttpMessage parseHead(
            final SessionInputBuffer sessionBuffer) throws IOException, HttpException {
        
        int count = 0;
        ParserCursor cursor = null;
        do {
            
            this.lineBuf.clear();
            final int i = sessionBuffer.readLine(this.lineBuf);
            if (i == -1 && count == 0) {
                
                throw new NoHttpResponseException("The target server failed to respond");
            }
            cursor = new ParserCursor(0, this.lineBuf.length());
            if (lineParser.hasProtocolVersion(this.lineBuf, cursor)) {
                
                break;
            } else if (i == -1 || count >= this.maxGarbageLines) {
                
                throw new ProtocolException("The server failed to respond with a " +
                        "valid HTTP response");
            }
            if (this.log.isDebugEnabled()) {
                this.log.debug("Garbage in response: " + this.lineBuf.toString());
            }
            count++;
        } while(true);
        
        final StatusLine statusline = lineParser.parseStatusLine(this.lineBuf, cursor);
        return this.responseFactory.newHttpResponse(statusline, null);
    }

}
