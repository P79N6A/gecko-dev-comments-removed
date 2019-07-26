

















package org.mozilla.apache.commons.codec.language;

import java.util.Locale;

import org.mozilla.apache.commons.codec.EncoderException;
import org.mozilla.apache.commons.codec.StringEncoder;































































































































































public class ColognePhonetic implements StringEncoder {

    private abstract class CologneBuffer {

        protected final char[] data;

        protected int length = 0;

        public CologneBuffer(char[] data) {
            this.data = data;
            this.length = data.length;
        }

        public CologneBuffer(int buffSize) {
            this.data = new char[buffSize];
            this.length = 0;
        }

        protected abstract char[] copyData(int start, final int length);

        public int length() {
            return length;
        }

        public String toString() {
            return new String(copyData(0, length));
        }
    }

    private class CologneOutputBuffer extends CologneBuffer {

        public CologneOutputBuffer(int buffSize) {
            super(buffSize);
        }

        public void addRight(char chr) {
            data[length] = chr;
            length++;
        }

        protected char[] copyData(int start, final int length) {
            char[] newData = new char[length];
            System.arraycopy(data, start, newData, 0, length);
            return newData;
        }
    }

    private class CologneInputBuffer extends CologneBuffer {

        public CologneInputBuffer(char[] data) {
            super(data);
        }

        public void addLeft(char ch) {
            length++;
            data[getNextPos()] = ch;
        }

        protected char[] copyData(int start, final int length) {
            char[] newData = new char[length];
            System.arraycopy(data, data.length - this.length + start, newData, 0, length);
            return newData;
        }

        public char getNextChar() {
            return data[getNextPos()];
        }

        protected int getNextPos() {
            return data.length - length;
        }

        public char removeNext() {
            char ch = getNextChar();
            length--;
            return ch;
        }
    }

    private static final char[][] PREPROCESS_MAP = new char[][]{{'\u00C4', 'A'}, 
        {'\u00DC', 'U'}, 
        {'\u00D6', 'O'}, 
        {'\u00DF', 'S'} 
    };

    


    private static boolean arrayContains(char[] arr, char key) {
        for (int i = 0; i < arr.length; i++) {
            if (arr[i] == key) {
                return true;
            }
        }
        return false;
    }

    










    public String colognePhonetic(String text) {
        if (text == null) {
            return null;
        }

        text = preprocess(text);

        CologneOutputBuffer output = new CologneOutputBuffer(text.length() * 2);
        CologneInputBuffer input = new CologneInputBuffer(text.toCharArray());

        char nextChar;

        char lastChar = '-';
        char lastCode = '/';
        char code;
        char chr;

        int rightLength = input.length();

        while (rightLength > 0) {
            chr = input.removeNext();

            if ((rightLength = input.length()) > 0) {
                nextChar = input.getNextChar();
            } else {
                nextChar = '-';
            }

            if (arrayContains(new char[]{'A', 'E', 'I', 'J', 'O', 'U', 'Y'}, chr)) {
                code = '0';
            } else if (chr == 'H' || chr < 'A' || chr > 'Z') {
                if (lastCode == '/') {
                    continue;
                }
                code = '-';
            } else if (chr == 'B' || (chr == 'P' && nextChar != 'H')) {
                code = '1';
            } else if ((chr == 'D' || chr == 'T') && !arrayContains(new char[]{'S', 'C', 'Z'}, nextChar)) {
                code = '2';
            } else if (arrayContains(new char[]{'W', 'F', 'P', 'V'}, chr)) {
                code = '3';
            } else if (arrayContains(new char[]{'G', 'K', 'Q'}, chr)) {
                code = '4';
            } else if (chr == 'X' && !arrayContains(new char[]{'C', 'K', 'Q'}, lastChar)) {
                code = '4';
                input.addLeft('S');
                rightLength++;
            } else if (chr == 'S' || chr == 'Z') {
                code = '8';
            } else if (chr == 'C') {
                if (lastCode == '/') {
                    if (arrayContains(new char[]{'A', 'H', 'K', 'L', 'O', 'Q', 'R', 'U', 'X'}, nextChar)) {
                        code = '4';
                    } else {
                        code = '8';
                    }
                } else {
                    if (arrayContains(new char[]{'S', 'Z'}, lastChar) ||
                        !arrayContains(new char[]{'A', 'H', 'O', 'U', 'K', 'Q', 'X'}, nextChar)) {
                        code = '8';
                    } else {
                        code = '4';
                    }
                }
            } else if (arrayContains(new char[]{'T', 'D', 'X'}, chr)) {
                code = '8';
            } else if (chr == 'R') {
                code = '7';
            } else if (chr == 'L') {
                code = '5';
            } else if (chr == 'M' || chr == 'N') {
                code = '6';
            } else {
                code = chr;
            }

            if (code != '-' && (lastCode != code && (code != '0' || lastCode == '/') || code < '0' || code > '8')) {
                output.addRight(code);
            }

            lastChar = chr;
            lastCode = code;
        }
        return output.toString();
    }

    public Object encode(Object object) throws EncoderException {
        if (!(object instanceof String)) {
            throw new EncoderException("This methodâ€™s parameter was expected to be of the type " +
                String.class.getName() +
                ". But actually it was of the type " +
                object.getClass().getName() +
                ".");
        }
        return encode((String) object);
    }

    public String encode(String text) {
        return colognePhonetic(text);
    }

    public boolean isEncodeEqual(String text1, String text2) {
        return colognePhonetic(text1).equals(colognePhonetic(text2));
    }

    


    private String preprocess(String text) {
        text = text.toUpperCase(Locale.GERMAN);

        char[] chrs = text.toCharArray();

        for (int index = 0; index < chrs.length; index++) {
            if (chrs[index] > 'Z') {
                for (int replacement = 0; replacement < PREPROCESS_MAP.length; replacement++) {
                    if (chrs[index] == PREPROCESS_MAP[replacement][0]) {
                        chrs[index] = PREPROCESS_MAP[replacement][1];
                        break;
                    }
                }
            }
        }
        return new String(chrs);
    }
}
