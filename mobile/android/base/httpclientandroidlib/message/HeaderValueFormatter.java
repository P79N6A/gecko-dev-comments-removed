


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;
















public interface HeaderValueFormatter {

    












    CharArrayBuffer formatElements(CharArrayBuffer buffer,
                                   HeaderElement[] elems,
                                   boolean quote);

    












    CharArrayBuffer formatHeaderElement(CharArrayBuffer buffer,
                                        HeaderElement elem,
                                        boolean quote);

    














    CharArrayBuffer formatParameters(CharArrayBuffer buffer,
                                     NameValuePair[] nvps,
                                     boolean quote);

    












    CharArrayBuffer formatNameValuePair(CharArrayBuffer buffer,
                                        NameValuePair nvp,
                                        boolean quote);

}

