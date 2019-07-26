

















package org.mozilla.apache.commons.codec.language;

import org.mozilla.apache.commons.codec.EncoderException;
import org.mozilla.apache.commons.codec.StringEncoder;














public class Caverphone implements StringEncoder {

    


    final private Caverphone2 encoder = new Caverphone2();

    


    public Caverphone() {
        super();
    }

    






    public String caverphone(String source) {
        return this.encoder.encode(source);
    }

    










    public Object encode(Object pObject) throws EncoderException {
        if (!(pObject instanceof String)) {
            throw new EncoderException("Parameter supplied to Caverphone encode is not of type java.lang.String");
        }
        return this.caverphone((String) pObject);
    }

    






    public String encode(String pString) {
        return this.caverphone(pString);
    }

    








    public boolean isCaverphoneEqual(String str1, String str2) {
        return this.caverphone(str1).equals(this.caverphone(str2));
    }

}
