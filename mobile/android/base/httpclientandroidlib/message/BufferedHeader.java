


























package ch.boye.httpclientandroidlib.message;

import java.io.Serializable;

import ch.boye.httpclientandroidlib.FormattedHeader;
import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;







public class BufferedHeader implements FormattedHeader, Cloneable, Serializable {

    private static final long serialVersionUID = -2768352615787625448L;

    


    private final String name;

    


    private final CharArrayBuffer buffer;

    


    private final int valuePos;


    








    public BufferedHeader(final CharArrayBuffer buffer)
        throws ParseException {

        super();
        if (buffer == null) {
            throw new IllegalArgumentException
                ("Char array buffer may not be null");
        }
        int colon = buffer.indexOf(':');
        if (colon == -1) {
            throw new ParseException
                ("Invalid header: " + buffer.toString());
        }
        String s = buffer.substringTrimmed(0, colon);
        if (s.length() == 0) {
            throw new ParseException
                ("Invalid header: " + buffer.toString());
        }
        this.buffer = buffer;
        this.name = s;
        this.valuePos = colon + 1;
    }


    public String getName() {
        return this.name;
    }

    public String getValue() {
        return this.buffer.substringTrimmed(this.valuePos, this.buffer.length());
    }

    public HeaderElement[] getElements() throws ParseException {
        ParserCursor cursor = new ParserCursor(0, this.buffer.length());
        cursor.updatePos(this.valuePos);
        return BasicHeaderValueParser.DEFAULT
            .parseElements(this.buffer, cursor);
    }

    public int getValuePos() {
        return this.valuePos;
    }

    public CharArrayBuffer getBuffer() {
        return this.buffer;
    }

    public String toString() {
        return this.buffer.toString();
    }

    public Object clone() throws CloneNotSupportedException {
        
        
        return super.clone();
    }

}
