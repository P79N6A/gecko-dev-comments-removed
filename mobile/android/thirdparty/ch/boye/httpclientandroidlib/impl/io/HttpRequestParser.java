


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.ConnectionClosedException;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.HttpRequestFactory;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.LineParser;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
















@Deprecated
@NotThreadSafe
public class HttpRequestParser extends AbstractMessageParser<HttpMessage> {

    private final HttpRequestFactory requestFactory;
    private final CharArrayBuffer lineBuf;

    








    public HttpRequestParser(
            final SessionInputBuffer buffer,
            final LineParser parser,
            final HttpRequestFactory requestFactory,
            final HttpParams params) {
        super(buffer, parser, params);
        this.requestFactory = Args.notNull(requestFactory, "Request factory");
        this.lineBuf = new CharArrayBuffer(128);
    }

    @Override
    protected HttpMessage parseHead(
            final SessionInputBuffer sessionBuffer)
        throws IOException, HttpException, ParseException {

        this.lineBuf.clear();
        final int i = sessionBuffer.readLine(this.lineBuf);
        if (i == -1) {
            throw new ConnectionClosedException("Client closed connection");
        }
        final ParserCursor cursor = new ParserCursor(0, this.lineBuf.length());
        final RequestLine requestline = this.lineParser.parseRequestLine(this.lineBuf, cursor);
        return this.requestFactory.newHttpRequest(requestline);
    }

}
