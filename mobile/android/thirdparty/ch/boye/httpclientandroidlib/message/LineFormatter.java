


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
























public interface LineFormatter {

    
















    CharArrayBuffer appendProtocolVersion(CharArrayBuffer buffer,
                                          ProtocolVersion version);

    









    CharArrayBuffer formatRequestLine(CharArrayBuffer buffer,
                                      RequestLine reqline);

    











    CharArrayBuffer formatStatusLine(CharArrayBuffer buffer,
                                     StatusLine statline);

    


















    CharArrayBuffer formatHeader(CharArrayBuffer buffer,
                                 Header header);

}
