


























package ch.boye.httpclientandroidlib.impl.cookie;

import java.util.ArrayList;
import java.util.List;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.message.BasicHeaderElement;
import ch.boye.httpclientandroidlib.message.BasicNameValuePair;
import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;





@Immutable
public class NetscapeDraftHeaderParser {

    public final static NetscapeDraftHeaderParser DEFAULT = new NetscapeDraftHeaderParser();

    public NetscapeDraftHeaderParser() {
        super();
    }

    public HeaderElement parseHeader(
            final CharArrayBuffer buffer,
            final ParserCursor cursor) throws ParseException {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final NameValuePair nvp = parseNameValuePair(buffer, cursor);
        final List<NameValuePair> params = new ArrayList<NameValuePair>();
        while (!cursor.atEnd()) {
            final NameValuePair param = parseNameValuePair(buffer, cursor);
            params.add(param);
        }
        return new BasicHeaderElement(
                nvp.getName(),
                nvp.getValue(), params.toArray(new NameValuePair[params.size()]));
    }

    private NameValuePair parseNameValuePair(
            final CharArrayBuffer buffer, final ParserCursor cursor) {
        boolean terminated = false;

        int pos = cursor.getPos();
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        
        String name = null;
        while (pos < indexTo) {
            final char ch = buffer.charAt(pos);
            if (ch == '=') {
                break;
            }
            if (ch == ';') {
                terminated = true;
                break;
            }
            pos++;
        }

        if (pos == indexTo) {
            terminated = true;
            name = buffer.substringTrimmed(indexFrom, indexTo);
        } else {
            name = buffer.substringTrimmed(indexFrom, pos);
            pos++;
        }

        if (terminated) {
            cursor.updatePos(pos);
            return new BasicNameValuePair(name, null);
        }

        
        String value = null;
        int i1 = pos;

        while (pos < indexTo) {
            final char ch = buffer.charAt(pos);
            if (ch == ';') {
                terminated = true;
                break;
            }
            pos++;
        }

        int i2 = pos;
        
        while (i1 < i2 && (HTTP.isWhitespace(buffer.charAt(i1)))) {
            i1++;
        }
        
        while ((i2 > i1) && (HTTP.isWhitespace(buffer.charAt(i2 - 1)))) {
            i2--;
        }
        value = buffer.substring(i1, i2);
        if (terminated) {
            pos++;
        }
        cursor.updatePos(pos);
        return new BasicNameValuePair(name, value);
    }

}
