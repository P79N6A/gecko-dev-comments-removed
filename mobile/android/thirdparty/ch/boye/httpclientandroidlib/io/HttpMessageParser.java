


























package ch.boye.httpclientandroidlib.io;

import java.io.IOException;

import ch.boye.httpclientandroidlib.HttpException;
import ch.boye.httpclientandroidlib.HttpMessage;









public interface HttpMessageParser<T extends HttpMessage> {

    







    T parse()
        throws IOException, HttpException;

}
