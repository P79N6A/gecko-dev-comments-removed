


























package ch.boye.httpclientandroidlib.message;

import ch.boye.httpclientandroidlib.Header;
import ch.boye.httpclientandroidlib.HttpVersion;
import ch.boye.httpclientandroidlib.ParseException;
import ch.boye.httpclientandroidlib.ProtocolVersion;
import ch.boye.httpclientandroidlib.RequestLine;
import ch.boye.httpclientandroidlib.StatusLine;
import ch.boye.httpclientandroidlib.annotation.Immutable;
import ch.boye.httpclientandroidlib.protocol.HTTP;
import ch.boye.httpclientandroidlib.util.Args;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;



















@Immutable
public class BasicLineParser implements LineParser {

    







    @Deprecated
    public final static BasicLineParser DEFAULT = new BasicLineParser();

    public final static BasicLineParser INSTANCE = new BasicLineParser();

    



    protected final ProtocolVersion protocol;


    






    public BasicLineParser(final ProtocolVersion proto) {
        this.protocol = proto != null? proto : HttpVersion.HTTP_1_1;
    }


    


    public BasicLineParser() {
        this(null);
    }


    public static
        ProtocolVersion parseProtocolVersion(final String value,
                                             final LineParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicLineParser.INSTANCE)
                .parseProtocolVersion(buffer, cursor);
    }


    
    public ProtocolVersion parseProtocolVersion(final CharArrayBuffer buffer,
                                                final ParserCursor cursor) throws ParseException {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final String protoname = this.protocol.getProtocol();
        final int protolength  = protoname.length();

        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        skipWhitespace(buffer, cursor);

        int i = cursor.getPos();

        
        if (i + protolength + 4 > indexTo) {
            throw new ParseException
                ("Not a valid protocol version: " +
                 buffer.substring(indexFrom, indexTo));
        }

        
        boolean ok = true;
        for (int j=0; ok && (j<protolength); j++) {
            ok = (buffer.charAt(i+j) == protoname.charAt(j));
        }
        if (ok) {
            ok = (buffer.charAt(i+protolength) == '/');
        }
        if (!ok) {
            throw new ParseException
                ("Not a valid protocol version: " +
                 buffer.substring(indexFrom, indexTo));
        }

        i += protolength+1;

        final int period = buffer.indexOf('.', i, indexTo);
        if (period == -1) {
            throw new ParseException
                ("Invalid protocol version number: " +
                 buffer.substring(indexFrom, indexTo));
        }
        final int major;
        try {
            major = Integer.parseInt(buffer.substringTrimmed(i, period));
        } catch (final NumberFormatException e) {
            throw new ParseException
                ("Invalid protocol major version number: " +
                 buffer.substring(indexFrom, indexTo));
        }
        i = period + 1;

        int blank = buffer.indexOf(' ', i, indexTo);
        if (blank == -1) {
            blank = indexTo;
        }
        final int minor;
        try {
            minor = Integer.parseInt(buffer.substringTrimmed(i, blank));
        } catch (final NumberFormatException e) {
            throw new ParseException(
                "Invalid protocol minor version number: " +
                buffer.substring(indexFrom, indexTo));
        }

        cursor.updatePos(blank);

        return createProtocolVersion(major, minor);

    } 


    








    protected ProtocolVersion createProtocolVersion(final int major, final int minor) {
        return protocol.forVersion(major, minor);
    }



    
    public boolean hasProtocolVersion(final CharArrayBuffer buffer,
                                      final ParserCursor cursor) {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        int index = cursor.getPos();

        final String protoname = this.protocol.getProtocol();
        final int  protolength = protoname.length();

        if (buffer.length() < protolength+4)
         {
            return false; 
        }

        if (index < 0) {
            
            
            index = buffer.length() -4 -protolength;
        } else if (index == 0) {
            
            while ((index < buffer.length()) &&
                    HTTP.isWhitespace(buffer.charAt(index))) {
                 index++;
             }
        } 


        if (index + protolength + 4 > buffer.length()) {
            return false;
        }


        
        boolean ok = true;
        for (int j=0; ok && (j<protolength); j++) {
            ok = (buffer.charAt(index+j) == protoname.charAt(j));
        }
        if (ok) {
            ok = (buffer.charAt(index+protolength) == '/');
        }

        return ok;
    }



    public static
        RequestLine parseRequestLine(final String value,
                                     final LineParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicLineParser.INSTANCE)
            .parseRequestLine(buffer, cursor);
    }


    








    public RequestLine parseRequestLine(final CharArrayBuffer buffer,
                                        final ParserCursor cursor) throws ParseException {

        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        try {
            skipWhitespace(buffer, cursor);
            int i = cursor.getPos();

            int blank = buffer.indexOf(' ', i, indexTo);
            if (blank < 0) {
                throw new ParseException("Invalid request line: " +
                        buffer.substring(indexFrom, indexTo));
            }
            final String method = buffer.substringTrimmed(i, blank);
            cursor.updatePos(blank);

            skipWhitespace(buffer, cursor);
            i = cursor.getPos();

            blank = buffer.indexOf(' ', i, indexTo);
            if (blank < 0) {
                throw new ParseException("Invalid request line: " +
                        buffer.substring(indexFrom, indexTo));
            }
            final String uri = buffer.substringTrimmed(i, blank);
            cursor.updatePos(blank);

            final ProtocolVersion ver = parseProtocolVersion(buffer, cursor);

            skipWhitespace(buffer, cursor);
            if (!cursor.atEnd()) {
                throw new ParseException("Invalid request line: " +
                        buffer.substring(indexFrom, indexTo));
            }

            return createRequestLine(method, uri, ver);
        } catch (final IndexOutOfBoundsException e) {
            throw new ParseException("Invalid request line: " +
                                     buffer.substring(indexFrom, indexTo));
        }
    } 


    









    protected RequestLine createRequestLine(final String method,
                                            final String uri,
                                            final ProtocolVersion ver) {
        return new BasicRequestLine(method, uri, ver);
    }



    public static
        StatusLine parseStatusLine(final String value,
                                   final LineParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        final ParserCursor cursor = new ParserCursor(0, value.length());
        return (parser != null ? parser : BasicLineParser.INSTANCE)
                .parseStatusLine(buffer, cursor);
    }


    
    public StatusLine parseStatusLine(final CharArrayBuffer buffer,
                                      final ParserCursor cursor) throws ParseException {
        Args.notNull(buffer, "Char array buffer");
        Args.notNull(cursor, "Parser cursor");
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();

        try {
            
            final ProtocolVersion ver = parseProtocolVersion(buffer, cursor);

            
            skipWhitespace(buffer, cursor);
            int i = cursor.getPos();

            int blank = buffer.indexOf(' ', i, indexTo);
            if (blank < 0) {
                blank = indexTo;
            }
            final int statusCode;
            final String s = buffer.substringTrimmed(i, blank);
            for (int j = 0; j < s.length(); j++) {
                if (!Character.isDigit(s.charAt(j))) {
                    throw new ParseException(
                            "Status line contains invalid status code: "
                            + buffer.substring(indexFrom, indexTo));
                }
            }
            try {
                statusCode = Integer.parseInt(s);
            } catch (final NumberFormatException e) {
                throw new ParseException(
                        "Status line contains invalid status code: "
                        + buffer.substring(indexFrom, indexTo));
            }
            
            i = blank;
            final String reasonPhrase;
            if (i < indexTo) {
                reasonPhrase = buffer.substringTrimmed(i, indexTo);
            } else {
                reasonPhrase = "";
            }
            return createStatusLine(ver, statusCode, reasonPhrase);

        } catch (final IndexOutOfBoundsException e) {
            throw new ParseException("Invalid status line: " +
                                     buffer.substring(indexFrom, indexTo));
        }
    } 


    









    protected StatusLine createStatusLine(final ProtocolVersion ver,
                                          final int status,
                                          final String reason) {
        return new BasicStatusLine(ver, status, reason);
    }



    public static
        Header parseHeader(final String value,
                           final LineParser parser) throws ParseException {
        Args.notNull(value, "Value");

        final CharArrayBuffer buffer = new CharArrayBuffer(value.length());
        buffer.append(value);
        return (parser != null ? parser : BasicLineParser.INSTANCE)
                .parseHeader(buffer);
    }


    
    public Header parseHeader(final CharArrayBuffer buffer)
        throws ParseException {

        
        return new BufferedHeader(buffer);
    }


    


    protected void skipWhitespace(final CharArrayBuffer buffer, final ParserCursor cursor) {
        int pos = cursor.getPos();
        final int indexTo = cursor.getUpperBound();
        while ((pos < indexTo) &&
               HTTP.isWhitespace(buffer.charAt(pos))) {
            pos++;
        }
        cursor.updatePos(pos);
    }

} 
