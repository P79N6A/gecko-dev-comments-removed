


























package ch.boye.httpclientandroidlib.message;

import java.util.ArrayList;
import java.util.List;

import ch.boye.httpclientandroidlib.HeaderElement;
import ch.boye.httpclientandroidlib.NameValuePair;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;








@Immutable
public class BasicHeaderValueParser implements HeaderValueParser {

    







    @Deprecated
    public final static
        BasicHeaderValueParser DEFAULT = new BasicHeaderValueParser();

    public final static BasicHeaderValueParser INSTANCE = new BasicHeaderValueParser();

    private final static char PARAM_DELIMITER                = ';';
    private final static char ELEM_DELIMITER                 = ',';
    private final static char[] ALL_DELIMITERS               = new char[] {
                                                                PARAM_DELIMITER,
                                                                ELEM_DELIMITER
                                                                };

    public BasicHeaderValueParser() {
        super();
    }

    







    public static
        HeaderElement[] parseElements(final String value,
                                      final HeaderValueParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicHeaderValueParser.INSTANCE)
            .parseElements(buffer, cursor);
    }


    
    public HeaderElement[] parseElements(final CharArrayBuffer buffer,
                                         final ParserCursor cursor) {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final List<HeaderElement> elements = new ArrayList<HeaderElement>();
        while (!cursor.atEnd()) {
            final HeaderElement element = parseHeaderElement(buffer, cursor);
            if (!(element.getName().length() == 0 && element.getValue() == null)) {
                elements.add(element);
            }
        }
        return elements.toArray(new HeaderElement[elements.size()]);
    }


    







    public static
        HeaderElement parseHeaderElement(final String value,
                                         final HeaderValueParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicHeaderValueParser.INSTANCE)
                .parseHeaderElement(buffer, cursor);
    }


    
    public HeaderElement parseHeaderElement(final CharArrayBuffer buffer,
                                            final ParserCursor cursor) {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final NameValuePair nvp = parseNameValuePair(buffer, cursor);
        NameValuePair[] params = null;
        if (!cursor.atEnd()) {
            final char ch = buffer.charAt(cursor.getPos() - 1);
            if (ch != ELEM_DELIMITER) {
                params = parseParameters(buffer, cursor);
            }
        }
        return createHeaderElement(nvp.getName(), nvp.getValue(), params);
    }


    





    protected HeaderElement createHeaderElement(
            final String name,
            final String value,
            final NameValuePair[] params) {
        return new BasicHeaderElement(name, value, params);
    }


    







    public static
        NameValuePair[] parseParameters(final String value,
                                        final HeaderValueParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicHeaderValueParser.INSTANCE)
                .parseParameters(buffer, cursor);
    }



    
    public NameValuePair[] parseParameters(final CharArrayBuffer buffer,
                                           final ParserCursor cursor) {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        int pos = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        while (pos < indexTo) {
            final char ch = buffer.charAt(pos);
            if (HTTP.isWhitespace(ch)) {
                pos++;
            } else {
                break;
            }
        }
        cursor.updatePos(pos);
        if (cursor.atEnd()) {
            return new NameValuePair[] {};
        }

        final List<NameValuePair> params = new ArrayList<NameValuePair>();
        while (!cursor.atEnd()) {
            final NameValuePair param = parseNameValuePair(buffer, cursor);
            params.add(param);
            final char ch = buffer.charAt(cursor.getPos() - 1);
            if (ch == ELEM_DELIMITER) {
                break;
            }
        }

        return params.toArray(new NameValuePair[params.size()]);
    }

    







    public static
       NameValuePair parseNameValuePair(final String value,
                                        final HeaderValueParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicHeaderValueParser.INSTANCE)
                .parseNameValuePair(buffer, cursor);
    }


    
    public NameValuePair parseNameValuePair(final CharArrayBuffer buffer,
                                            final ParserCursor cursor) {
        return parseNameValuePair(buffer, cursor, ALL_DELIMITERS);
    }

    private static boolean isOneOf(final char ch, final char[] chs) {
        if (chs != null) {
            for (final char ch2 : chs) {
                if (ch == ch2) {
                    return true;
                }
            }
        }
        return false;
    }

    public NameValuePair parseNameValuePair(final CharArrayBuffer buffer,
                                            final ParserCursor cursor,
                                            final char[] delimiters) {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");

        boolean terminated = false;

        int pos = cursor.getPos();
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        
        final String name;
        while (pos < indexTo) {
            final char ch = buffer.charAt(pos);
            if (ch == '=') {
                break;
            }
            if (isOneOf(ch, delimiters)) {
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
            return createNameValuePair(name, null);
        }

        
        final String value;
        int i1 = pos;

        boolean qouted = false;
        boolean escaped = false;
        while (pos < indexTo) {
            final char ch = buffer.charAt(pos);
            if (ch == '"' && !escaped) {
                qouted = !qouted;
            }
            if (!qouted && !escaped && isOneOf(ch, delimiters)) {
                terminated = true;
                break;
            }
            if (escaped) {
                escaped = false;
            } else {
                escaped = qouted && ch == '\\';
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
        
        if (((i2 - i1) >= 2)
            && (buffer.charAt(i1) == '"')
            && (buffer.charAt(i2 - 1) == '"')) {
            i1++;
            i2--;
        }
        value = buffer.substring(i1, i2);
        if (terminated) {
            pos++;
        }
        cursor.updatePos(pos);
        return createNameValuePair(name, value);
    }

    








    protected NameValuePair createNameValuePair(final String name, final String value) {
        return new BasicNameValuePair(name, value);
    }

}

