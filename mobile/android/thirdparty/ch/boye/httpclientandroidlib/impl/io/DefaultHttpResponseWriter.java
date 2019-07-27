


























package ch.boye.httpclientandroidlib.impl.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpResponse;
import ch.boye.httpclientandroidlib.annotation.NotThreadSafe;
import ch.boye.httpclientandroidlib.io.SessionOutputBuffer;
import ch.boye.httpclientandroidlib.message.LineFormatter;






@NotThreadSafe
public class DefaultHttpResponseWriter extends AbstractMessageWriter<HttpResponse> {

    







    public DefaultHttpResponseWriter(
            final SessionOutputBuffer buffer,
            final LineFormatter formatter) {
        super(buffer, formatter);
    }

    public DefaultHttpResponseWriter(final SessionOutputBuffer buffer) {
        super(buffer, null);
    }

    @Override
    protected void writeHeadLine(final HttpResponse message) throws IOException {
        lineFormatter.formatStatusLine(this.lineBuf, message.getStatusLine());
        this.sessionBuffer.writeLine(this.lineBuf);
    }

}
