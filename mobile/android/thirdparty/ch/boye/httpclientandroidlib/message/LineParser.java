


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;











public interface LineParser {

    












    ProtocolVersion parseProtocolVersion(
            CharArrayBuffer buffer,
            ParserCursor cursor) throws ParseException;

    


















    boolean hasProtocolVersion(
            CharArrayBuffer buffer,
            ParserCursor cursor);

    










    RequestLine parseRequestLine(
            CharArrayBuffer buffer,
            ParserCursor cursor) throws ParseException;

    










    StatusLine parseStatusLine(
            CharArrayBuffer buffer,
            ParserCursor cursor) throws ParseException;

    














    Header parseHeader(CharArrayBuffer buffer)
        throws ParseException;

}
