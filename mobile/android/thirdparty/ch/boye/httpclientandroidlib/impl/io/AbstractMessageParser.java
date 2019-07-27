


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.MessageConstraintException;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolException;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.config.MessageConstraints;
import ch.boye.httpclientandroidlib.io.HttpMessageParser;
import ch.boye.httpclientandroidlib.io.SessionInputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineParser;
import ch.boye.httpclientandroidlib.message.LineParser;
import ch.boye.httpclientandroidlib.params.HttpParamConfig;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;







@SuppressWarnings("deprecation")
@NotThreadSafe
public abstract class AbstractMessageParser<T extends HttpMessage> implements HttpMessageParser<T> {

    private static final int HEAD_LINE    = 0;
    private static final int HEADERS      = 1;

    private final SessionInputBuffer sessionBuffer;
    private final MessageConstraints messageConstraints;
    private final List<CharArrayBuffer> headerLines;
    protected final LineParser lineParser;

    private int state;
    private T message;

    









    @Deprecated
    public AbstractMessageParser(
            final SessionInputBuffer buffer,
            final LineParser parser,
            final HttpParams params) {
        super();
        Args.notNull(buffer, "Session input buffer");
        Args.notNull(params, "HTTP parameters");
        this.sessionBuffer = buffer;
        this.messageConstraints = HttpParamConfig.getMessageConstraints(params);
        this.lineParser = (parser != null) ? parser : BasicLineParser.INSTANCE;
        this.headerLines = new ArrayList<CharArrayBuffer>();
        this.state = HEAD_LINE;
    }

    










    public AbstractMessageParser(
            final SessionInputBuffer buffer,
            final LineParser lineParser,
            final MessageConstraints constraints) {
        super();
        this.sessionBuffer = Args.notNull(buffer, "Session input buffer");
        this.lineParser = lineParser != null ? lineParser : BasicLineParser.INSTANCE;
        this.messageConstraints = constraints != null ? constraints : MessageConstraints.DEFAULT;
        this.headerLines = new ArrayList<CharArrayBuffer>();
        this.state = HEAD_LINE;
    }

    


















    public static Header[] parseHeaders(
            final SessionInputBuffer inbuffer,
            final int maxHeaderCount,
            final int maxLineLen,
            final LineParser parser) throws HttpException, IOException {
        final List<CharArrayBuffer> headerLines = new ArrayList<CharArrayBuffer>();
        return parseHeaders(inbuffer, maxHeaderCount, maxLineLen,
                parser != null ? parser : BasicLineParser.INSTANCE,
                headerLines);
    }

    























    public static Header[] parseHeaders(
            final SessionInputBuffer inbuffer,
            final int maxHeaderCount,
            final int maxLineLen,
            final LineParser parser,
            final List<CharArrayBuffer> headerLines) throws HttpException, IOException {
        Args.notNull(inbuffer, "Session input buffer");
        Args.notNull(parser, "Line parser");
        Args.notNull(headerLines, "Header line list");

        CharArrayBuffer current = null;
        CharArrayBuffer previous = null;
        for (;;) {
            if (current == null) {
                current = new CharArrayBuffer(64);
            } else {
                current.clear();
            }
            final int l = inbuffer.readLine(current);
            if (l == -1 || current.length() < 1) {
                break;
            }
            
            
            
            
            if ((current.charAt(0) == ' ' || current.charAt(0) == '\t') && previous != null) {
                
                
                int i = 0;
                while (i < current.length()) {
                    final char ch = current.charAt(i);
                    if (ch != ' ' && ch != '\t') {
                        break;
                    }
                    i++;
                }
                if (maxLineLen > 0
                        && previous.length() + 1 + current.length() - i > maxLineLen) {
                    throw new MessageConstraintException("Maximum line length limit exceeded");
                }
                previous.append(' ');
                previous.append(current, i, current.length() - i);
            } else {
                headerLines.add(current);
                previous = current;
                current = null;
            }
            if (maxHeaderCount > 0 && headerLines.size() >= maxHeaderCount) {
                throw new MessageConstraintException("Maximum header count exceeded");
            }
        }
        final Header[] headers = new Header[headerLines.size()];
        for (int i = 0; i < headerLines.size(); i++) {
            final CharArrayBuffer buffer = headerLines.get(i);
            try {
                headers[i] = parser.parseHeader(buffer);
            } catch (final ParseException ex) {
                throw new ProtocolException(ex.getMessage());
            }
        }
        return headers;
    }

    













    protected abstract T parseHead(SessionInputBuffer sessionBuffer)
        throws IOException, HttpException, ParseException;

    public T parse() throws IOException, HttpException {
        final int st = this.state;
        switch (st) {
        case HEAD_LINE:
            try {
                this.message = parseHead(this.sessionBuffer);
            } catch (final ParseException px) {
                throw new ProtocolException(px.getMessage(), px);
            }
            this.state = HEADERS;
            
        case HEADERS:
            final Header[] headers = AbstractMessageParser.parseHeaders(
                    this.sessionBuffer,
                    this.messageConstraints.getMaxHeaderCount(),
                    this.messageConstraints.getMaxLineLength(),
                    this.lineParser,
                    this.headerLines);
            this.message.setHeaders(headers);
            final T result = this.message;
            this.message = null;
            this.headerLines.clear();
            this.state = HEAD_LINE;
            return result;
        default:
            throw new IllegalStateException("Inconsistent parser state");
        }
    }

}
