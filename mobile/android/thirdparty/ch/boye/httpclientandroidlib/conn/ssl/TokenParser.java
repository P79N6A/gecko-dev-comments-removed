


























package ch.boye.httpclientandroidlib.conn.ssl;

import java.util.BitSet;

import ch.boye.httpclientandroidlib.message.ParserCursor;
import ch.boye.httpclientandroidlib.util.CharArrayBuffer;









class TokenParser {

    public static BitSet INIT_BITSET(final int ... b) {
        final BitSet bitset = new BitSet();
        for (final int aB : b) {
            bitset.set(aB);
        }
        return bitset;
    }

    
    public static final char CR = '\r';

    
    public static final char LF = '\n';

    
    public static final char SP = ' ';

    
    public static final char HT = '\t';

    
    public static final char DQUOTE = '\"';

    
    public static final char ESCAPE = '\\';

    public static boolean isWhitespace(final char ch) {
        return ch == SP || ch == HT || ch == CR || ch == LF;
    }

    public static final TokenParser INSTANCE = new TokenParser();

    








    public String parseToken(final CharArrayBuffer buf, final ParserCursor cursor, final BitSet delimiters) {
        final StringBuilder dst = new StringBuilder();
        boolean whitespace = false;
        while (!cursor.atEnd()) {
            final char current = buf.charAt(cursor.getPos());
            if (delimiters != null && delimiters.get(current)) {
                break;
            } else if (isWhitespace(current)) {
                skipWhiteSpace(buf, cursor);
                whitespace = true;
            } else {
                if (whitespace && dst.length() > 0) {
                    dst.append(' ');
                }
                copyContent(buf, cursor, delimiters, dst);
                whitespace = false;
            }
        }
        return dst.toString();
    }

    









    public String parseValue(final CharArrayBuffer buf, final ParserCursor cursor, final BitSet delimiters) {
        final StringBuilder dst = new StringBuilder();
        boolean whitespace = false;
        while (!cursor.atEnd()) {
            final char current = buf.charAt(cursor.getPos());
            if (delimiters != null && delimiters.get(current)) {
                break;
            } else if (isWhitespace(current)) {
                skipWhiteSpace(buf, cursor);
                whitespace = true;
            } else if (current == DQUOTE) {
                if (whitespace && dst.length() > 0) {
                    dst.append(' ');
                }
                copyQuotedContent(buf, cursor, dst);
                whitespace = false;
            } else {
                if (whitespace && dst.length() > 0) {
                    dst.append(' ');
                }
                copyUnquotedContent(buf, cursor, delimiters, dst);
                whitespace = false;
            }
        }
        return dst.toString();
    }

    






    public void skipWhiteSpace(final CharArrayBuffer buf, final ParserCursor cursor) {
        int pos = cursor.getPos();
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();
        for (int i = indexFrom; i < indexTo; i++) {
            final char current = buf.charAt(i);
            if (!isWhitespace(current)) {
                break;
            } else {
                pos++;
            }
        }
        cursor.updatePos(pos);
    }

    









    public void copyContent(final CharArrayBuffer buf, final ParserCursor cursor, final BitSet delimiters,
            final StringBuilder dst) {
        int pos = cursor.getPos();
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();
        for (int i = indexFrom; i < indexTo; i++) {
            final char current = buf.charAt(i);
            if ((delimiters != null && delimiters.get(current)) || isWhitespace(current)) {
                break;
            } else {
                pos++;
                dst.append(current);
            }
        }
        cursor.updatePos(pos);
    }

    









    public void copyUnquotedContent(final CharArrayBuffer buf, final ParserCursor cursor,
            final BitSet delimiters, final StringBuilder dst) {
        int pos = cursor.getPos();
        final int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();
        for (int i = indexFrom; i < indexTo; i++) {
            final char current = buf.charAt(i);
            if ((delimiters != null && delimiters.get(current))
                    || isWhitespace(current) || current == DQUOTE) {
                break;
            } else {
                pos++;
                dst.append(current);
            }
        }
        cursor.updatePos(pos);
    }

    






    public void copyQuotedContent(final CharArrayBuffer buf, final ParserCursor cursor,
            final StringBuilder dst) {
        if (cursor.atEnd()) {
            return;
        }
        int pos = cursor.getPos();
        int indexFrom = cursor.getPos();
        final int indexTo = cursor.getUpperBound();
        char current = buf.charAt(pos);
        if (current != DQUOTE) {
            return;
        }
        pos++;
        indexFrom++;
        boolean escaped = false;
        for (int i = indexFrom; i < indexTo; i++, pos++) {
            current = buf.charAt(i);
            if (escaped) {
                if (current != DQUOTE && current != ESCAPE) {
                    dst.append(ESCAPE);
                }
                dst.append(current);
                escaped = false;
            } else {
                if (current == DQUOTE) {
                    pos++;
                    break;
                }
                if (current == ESCAPE) {
                    escaped = true;
                } else if (current != CR && current != LF) {
                    dst.append(current);
                }
            }
        }
        cursor.updatePos(pos);
    }

}
