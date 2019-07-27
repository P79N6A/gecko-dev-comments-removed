


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.HttpResponseFactory;
import ch.boye.httpclientandroidlib.NoHttpResponseException;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.impl.DefaultHttpResponseFactory;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.LineParser;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;







@SuppressWarnings("deprecation")
@NotThreadSafe
public class DefaultHttpResponseParser extends AbstractMessageParser<HttpResponse> {

    private final HttpResponseFactory responseFactory;
    private final CharArrayBuffer lineBuf;

    












    @Deprecated
    public DefaultHttpResponseParser(
            final SessionInputBuffer buffer,
            final LineParser lineParser,
            final HttpResponseFactory responseFactory,
            final HttpParams params) {
        super(buffer, lineParser, params);
        this.responseFactory = Args.notNull(responseFactory, "Response factory");
        this.lineBuf = new CharArrayBuffer(128);
    }

    












    public DefaultHttpResponseParser(
            final SessionInputBuffer buffer,
            final LineParser lineParser,
            final HttpResponseFactory responseFactory,
            final MessageConstraints constraints) {
        super(buffer, lineParser, constraints);
        this.responseFactory = responseFactory != null ? responseFactory :
            DefaultHttpResponseFactory.INSTANCE;
        this.lineBuf = new CharArrayBuffer(128);
    }

    


    public DefaultHttpResponseParser(
            final SessionInputBuffer buffer,
            final MessageConstraints constraints) {
        this(buffer, null, null, constraints);
    }

    


    public DefaultHttpResponseParser(final SessionInputBuffer buffer) {
        this(buffer, null, null, MessageConstraints.DEFAULT);
    }

    @Override
    protected HttpResponse parseHead(
            final SessionInputBuffer sessionBuffer)
        throws IOException, HttpException, ParseException {

        this.lineBuf.clear();
        final int i = sessionBuffer.readLine(this.lineBuf);
        if (i == -1) {
            throw new NoHttpResponseException("The target server failed to respond");
        }
        
        final ParserCursor cursor = new ParserCursor(0, this.lineBuf.length());
        final StatusLine statusline = lineParser.parseStatusLine(this.lineBuf, cursor);
        return this.responseFactory.newHttpResponse(statusline, null);
    }

}
