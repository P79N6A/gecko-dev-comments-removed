

















package org.mozilla.apache.commons.codec.binary;

























public class Base32 extends BaseNCodec {

    




    private static final int BITS_PER_ENCODED_BYTE = 5;
    private static final int BYTES_PER_ENCODED_BLOCK = 8;
    private static final int BYTES_PER_UNENCODED_BLOCK = 5;

    




    private static final byte[] CHUNK_SEPARATOR = {'\r', '\n'};

    





    private static final byte[] DECODE_TABLE = {
         
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 63, 
            -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1, 
            -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,                     
    };

    



    private static final byte[] ENCODE_TABLE = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
            '2', '3', '4', '5', '6', '7',
    };

    





    private static final byte[] HEX_DECODE_TABLE = {
         
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 63, 
             0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, 
            -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
            25, 26, 27, 28, 29, 30, 31, 32,                                 
    };

    



    private static final byte[] HEX_ENCODE_TABLE = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    };

    
    private static final int MASK_5BITS = 0x1f;

    
    
    

    



    private long bitWorkArea;

    



    private final int decodeSize;

    


    private final byte[] decodeTable;

    



    private final int encodeSize;

    


    private final byte[] encodeTable;

    


    private final byte[] lineSeparator;

    






    public Base32() {
        this(false);
    }

    






    public Base32(boolean useHex) {
        this(0, null, useHex);
    }

    









    public Base32(int lineLength) {
        this(lineLength, CHUNK_SEPARATOR);
    }

    
















    public Base32(int lineLength, byte[] lineSeparator) {
        this(lineLength, lineSeparator, false);
    }
    
    


















    public Base32(int lineLength, byte[] lineSeparator, boolean useHex) {
        super(BYTES_PER_UNENCODED_BLOCK, BYTES_PER_ENCODED_BLOCK, 
                lineLength, 
                lineSeparator == null ? 0 : lineSeparator.length);
        if (useHex){
            this.encodeTable = HEX_ENCODE_TABLE;
            this.decodeTable = HEX_DECODE_TABLE;            
        } else {
            this.encodeTable = ENCODE_TABLE;
            this.decodeTable = DECODE_TABLE;            
        }
        if (lineLength > 0) {
            if (lineSeparator == null) {
                throw new IllegalArgumentException("lineLength "+lineLength+" > 0, but lineSeparator is null");
            }
            
            if (containsAlphabetOrPad(lineSeparator)) {
                String sep = StringUtils.newStringUtf8(lineSeparator);
                throw new IllegalArgumentException("lineSeparator must not contain Base32 characters: [" + sep + "]");
            }
            this.encodeSize = BYTES_PER_ENCODED_BLOCK + lineSeparator.length;
            this.lineSeparator = new byte[lineSeparator.length];
            System.arraycopy(lineSeparator, 0, this.lineSeparator, 0, lineSeparator.length);
        } else {
            this.encodeSize = BYTES_PER_ENCODED_BLOCK;
            this.lineSeparator = null;
        }
        this.decodeSize = this.encodeSize - 1;
    }

    




















    void decode(byte[] in, int inPos, int inAvail) { 
        if (eof) {
            return;
        }
        if (inAvail < 0) {
            eof = true;
        }
        for (int i = 0; i < inAvail; i++) {
            byte b = in[inPos++];
            if (b == PAD) {
                
                eof = true;
                break;
            } else {
                ensureBufferSize(decodeSize);
                if (b >= 0 && b < this.decodeTable.length) {
                    int result = this.decodeTable[b];
                    if (result >= 0) {
                        modulus = (modulus+1) % BYTES_PER_ENCODED_BLOCK;
                        bitWorkArea = (bitWorkArea << BITS_PER_ENCODED_BYTE) + result; 
                        if (modulus == 0) { 
                            buffer[pos++] = (byte) ((bitWorkArea >> 32) & MASK_8BITS);
                            buffer[pos++] = (byte) ((bitWorkArea >> 24) & MASK_8BITS);
                            buffer[pos++] = (byte) ((bitWorkArea >> 16) & MASK_8BITS);
                            buffer[pos++] = (byte) ((bitWorkArea >> 8) & MASK_8BITS);
                            buffer[pos++] = (byte) (bitWorkArea & MASK_8BITS);
                        }
                    }
                }
            }
        }
    
        
        
        
        if (eof && modulus >= 2) { 
            ensureBufferSize(decodeSize);
    
            
            switch (modulus) {
                case 2 : 
                    buffer[pos++] = (byte) ((bitWorkArea >> 2) & MASK_8BITS);
                    break;
                case 3 : 
                    buffer[pos++] = (byte) ((bitWorkArea >> 7) & MASK_8BITS);
                    break;
                case 4 : 
                    bitWorkArea = bitWorkArea >> 4; 
                    buffer[pos++] = (byte) ((bitWorkArea >> 8) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea) & MASK_8BITS);
                    break;
                case 5 : 
                    bitWorkArea = bitWorkArea >> 1;
                    buffer[pos++] = (byte) ((bitWorkArea >> 16) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea >> 8) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea) & MASK_8BITS);
                    break;
                case 6 : 
                    bitWorkArea = bitWorkArea >> 6;
                    buffer[pos++] = (byte) ((bitWorkArea >> 16) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea >> 8) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea) & MASK_8BITS);
                    break;
                case 7 : 
                    bitWorkArea = bitWorkArea >> 3;
                    buffer[pos++] = (byte) ((bitWorkArea >> 24) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea >> 16) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea >> 8) & MASK_8BITS);
                    buffer[pos++] = (byte) ((bitWorkArea) & MASK_8BITS);
                    break;
            }
        }
    }

    













    void encode(byte[] in, int inPos, int inAvail) { 
        if (eof) {
            return;
        }
        
        
        if (inAvail < 0) {
            eof = true;
            if (0 == modulus && lineLength == 0) {
                return; 
            }
            ensureBufferSize(encodeSize);
            int savedPos = pos;
            switch (modulus) { 
                case 1 : 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 3) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea << 2) & MASK_5BITS]; 
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    break;
    
                case 2 : 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 11) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  6) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  1) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea <<  4) & MASK_5BITS]; 
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    break;
                case 3 : 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 19) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 14) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  9) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  4) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea <<  1) & MASK_5BITS]; 
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    buffer[pos++] = PAD;
                    break;
                case 4 : 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 27) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 22) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 17) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 12) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  7) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >>  2) & MASK_5BITS]; 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea <<  3) & MASK_5BITS]; 
                    buffer[pos++] = PAD;
                    break;
            }
            currentLinePos += pos - savedPos; 
            
            if (lineLength > 0 && currentLinePos > 0){ 
                System.arraycopy(lineSeparator, 0, buffer, pos, lineSeparator.length);
                pos += lineSeparator.length;
            }            
        } else {
            for (int i = 0; i < inAvail; i++) {
                ensureBufferSize(encodeSize);
                modulus = (modulus+1) % BYTES_PER_UNENCODED_BLOCK;
                int b = in[inPos++];
                if (b < 0) {
                    b += 256;
                }
                bitWorkArea = (bitWorkArea << 8) + b; 
                if (0 == modulus) { 
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 35) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 30) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 25) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 20) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 15) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 10) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)(bitWorkArea >> 5) & MASK_5BITS];
                    buffer[pos++] = encodeTable[(int)bitWorkArea & MASK_5BITS];
                    currentLinePos += BYTES_PER_ENCODED_BLOCK;
                    if (lineLength > 0 && lineLength <= currentLinePos) {
                        System.arraycopy(lineSeparator, 0, buffer, pos, lineSeparator.length);
                        pos += lineSeparator.length;
                        currentLinePos = 0;
                    }
                }
            }
        }
    }

    






    public boolean isInAlphabet(byte octet) {
        return octet >= 0 && octet < decodeTable.length && decodeTable[octet] != -1;
    }
}
