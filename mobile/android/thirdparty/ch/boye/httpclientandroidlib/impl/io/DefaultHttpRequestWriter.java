


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpRequest;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.LineFormatter;






@NotThreadSafe
public class DefaultHttpRequestWriter extends AbstractMessageWriter<HttpRequest> {

    







    public DefaultHttpRequestWriter(
            final SessionOutputBuffer buffer,
            final LineFormatter formatter) {
        super(buffer, formatter);
    }

    public DefaultHttpRequestWriter(final SessionOutputBuffer buffer) {
        this(buffer, null);
    }

    @Override
    protected void writeHeadLine(final HttpRequest message) throws IOException {
        lineFormatter.formatRequestLine(this.lineBuf, message.getRequestLine());
        this.sessionBuffer.writeLine(this.lineBuf);
    }

}
