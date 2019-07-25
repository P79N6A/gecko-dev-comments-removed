

















package org.mozilla.apache.commons.codec.language;

import org.mozilla.apache.commons.codec.EncoderException;
import org.mozilla.apache.commons.codec.StringEncoder;








public class Soundex implements StringEncoder {

    









    public static final String US_ENGLISH_MAPPING_STRING = "01230120022455012623010202";

    





    private static final char[] US_ENGLISH_MAPPING = US_ENGLISH_MAPPING_STRING.toCharArray();

    




    public static final Soundex US_ENGLISH = new Soundex();


    


















    public int difference(String s1, String s2) throws EncoderException {
        return SoundexUtils.difference(this, s1, s2);
    }

    




    private int maxLength = 4;

    



    private final char[] soundexMapping;

    





    public Soundex() {
        this.soundexMapping = US_ENGLISH_MAPPING;
    }

    









    public Soundex(char[] mapping) {
        this.soundexMapping = new char[mapping.length];
        System.arraycopy(mapping, 0, this.soundexMapping, 0, mapping.length);
    }

    







    public Soundex(String mapping) {
        this.soundexMapping = mapping.toCharArray();
    }

    












    public Object encode(Object pObject) throws EncoderException {
        if (!(pObject instanceof String)) {
            throw new EncoderException("Parameter supplied to Soundex encode is not of type java.lang.String");
        }
        return soundex((String) pObject);
    }

    








    public String encode(String pString) {
        return soundex(pString);
    }

    












    private char getMappingCode(String str, int index) {
        
        char mappedChar = this.map(str.charAt(index));
        
        if (index > 1 && mappedChar != '0') {
            char hwChar = str.charAt(index - 1);
            if ('H' == hwChar || 'W' == hwChar) {
                char preHWChar = str.charAt(index - 2);
                char firstCode = this.map(preHWChar);
                if (firstCode == mappedChar || 'H' == preHWChar || 'W' == preHWChar) {
                    return 0;
                }
            }
        }
        return mappedChar;
    }

    





    public int getMaxLength() {
        return this.maxLength;
    }

    




    private char[] getSoundexMapping() {
        return this.soundexMapping;
    }

    








    private char map(char ch) {
        int index = ch - 'A';
        if (index < 0 || index >= this.getSoundexMapping().length) {
            throw new IllegalArgumentException("The character is not mapped: " + ch);
        }
        return this.getSoundexMapping()[index];
    }

    






    public void setMaxLength(int maxLength) {
        this.maxLength = maxLength;
    }

    








    public String soundex(String str) {
        if (str == null) {
            return null;
        }
        str = SoundexUtils.clean(str);
        if (str.length() == 0) {
            return str;
        }
        char out[] = {'0', '0', '0', '0'};
        char last, mapped;
        int incount = 1, count = 1;
        out[0] = str.charAt(0);
        
        last = getMappingCode(str, 0);
        while ((incount < str.length()) && (count < out.length)) {
            mapped = getMappingCode(str, incount++);
            if (mapped != 0) {
                if ((mapped != '0') && (mapped != last)) {
                    out[count++] = mapped;
                }
                last = mapped;
            }
        }
        return new String(out);
    }

}
