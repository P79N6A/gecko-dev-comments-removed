


























package ch.boye.httpclientandroidlib.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;







public interface HttpMessageWriter<T extends HttpMessage> {

    







    void write(T message)
        throws IOException, HttpException;

}
