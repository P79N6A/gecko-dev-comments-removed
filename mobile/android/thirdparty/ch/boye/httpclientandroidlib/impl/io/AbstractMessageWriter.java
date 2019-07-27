


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HeaderIterator;
import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.HttpMessageWriter;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.BasicLineFormatter;
import ch.boye.httpclientandroidlib.message.LineFormatter;
import ch.boye.httpclientandroidlib.params.HttpParams;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;







@SuppressWarnings("deprecation")
@NotThreadSafe
public abstract class AbstractMessageWriter<T extends HttpMessage> implements HttpMessageWriter<T> {

    protected final SessionOutputBuffer sessionBuffer;
    protected final CharArrayBuffer lineBuf;
    protected final LineFormatter lineFormatter;

    









    @Deprecated
    public AbstractMessageWriter(final SessionOutputBuffer buffer,
                                 final LineFormatter formatter,
                                 final HttpParams params) {
        super();
        Args.notNull(buffer, "Session input buffer");
        this.sessionBuffer = buffer;
        this.lineBuf = new CharArrayBuffer(128);
        this.lineFormatter = (formatter != null) ? formatter : BasicLineFormatter.INSTANCE;
    }

    








    public AbstractMessageWriter(
            final SessionOutputBuffer buffer,
            final LineFormatter formatter) {
        super();
        this.sessionBuffer = Args.notNull(buffer, "Session input buffer");
        this.lineFormatter = (formatter != null) ? formatter : BasicLineFormatter.INSTANCE;
        this.lineBuf = new CharArrayBuffer(128);
    }

    






    protected abstract void writeHeadLine(T message) throws IOException;

    public void write(final T message) throws IOException, HttpException {
        Args.notNull(message, "HTTP message");
        writeHeadLine(message);
        for (final HeaderIterator it = message.headerIterator(); it.hasNext(); ) {
            final Header header = it.nextHeader();
            this.sessionBuffer.writeLine
                (lineFormatter.formatHeader(this.lineBuf, header));
        }
        this.lineBuf.clear();
        this.sessionBuffer.writeLine(this.lineBuf);
    }

}
