

























package ch.boye.httpclientandroidlib.impl.auth;

import java.security.Key;
import java.security.MessageDigest;
import java.util.Arrays;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

import org.mozilla.apache.commons.codec.binary.Base64;
import ch.boye.httpclientandroidlib.util.EncodingUtils;







final class NTLMEngineImpl implements NTLMEngine {

    
    protected final static int FLAG_UNICODE_ENCODING = 0x00000001;
    protected final static int FLAG_TARGET_DESIRED = 0x00000004;
    protected final static int FLAG_NEGOTIATE_SIGN = 0x00000010;
    protected final static int FLAG_NEGOTIATE_SEAL = 0x00000020;
    protected final static int FLAG_NEGOTIATE_NTLM = 0x00000200;
    protected final static int FLAG_NEGOTIATE_ALWAYS_SIGN = 0x00008000;
    protected final static int FLAG_NEGOTIATE_NTLM2 = 0x00080000;
    protected final static int FLAG_NEGOTIATE_128 = 0x20000000;
    protected final static int FLAG_NEGOTIATE_KEY_EXCH = 0x40000000;

    
    private static final java.security.SecureRandom RND_GEN;
    static {
        java.security.SecureRandom rnd = null;
        try {
            rnd = java.security.SecureRandom.getInstance("SHA1PRNG");
        } catch (Exception e) {
        }
        RND_GEN = rnd;
    }

    
    static final String DEFAULT_CHARSET = "ASCII";

    
    private String credentialCharset = DEFAULT_CHARSET;

    
    private static byte[] SIGNATURE;

    static {
        byte[] bytesWithoutNull = EncodingUtils.getBytes("NTLMSSP", "ASCII");
        SIGNATURE = new byte[bytesWithoutNull.length + 1];
        System.arraycopy(bytesWithoutNull, 0, SIGNATURE, 0, bytesWithoutNull.length);
        SIGNATURE[bytesWithoutNull.length] = (byte) 0x00;
    }

    
















    final String getResponseFor(String message, String username, String password,
            String host, String domain) throws NTLMEngineException {

        final String response;
        if (message == null || message.trim().equals("")) {
            response = getType1Message(host, domain);
        } else {
            Type2Message t2m = new Type2Message(message);
            response = getType3Message(username, password, host, domain, t2m.getChallenge(), t2m
                    .getFlags(), t2m.getTarget(), t2m.getTargetInfo());
        }
        return response;
    }

    










    String getType1Message(String host, String domain) throws NTLMEngineException {
        return new Type1Message(domain, host).getResponse();
    }

    



















    String getType3Message(String user, String password, String host, String domain,
            byte[] nonce, int type2Flags, String target, byte[] targetInformation)
            throws NTLMEngineException {
        return new Type3Message(domain, host, user, password, nonce, type2Flags, target,
                targetInformation).getResponse();
    }

    


    String getCredentialCharset() {
        return credentialCharset;
    }

    



    void setCredentialCharset(String credentialCharset) {
        this.credentialCharset = credentialCharset;
    }

    
    private static String stripDotSuffix(String value) {
        int index = value.indexOf(".");
        if (index != -1)
            return value.substring(0, index);
        return value;
    }

    
    private static String convertHost(String host) {
        return stripDotSuffix(host);
    }

    
    private static String convertDomain(String domain) {
        return stripDotSuffix(domain);
    }

    private static int readULong(byte[] src, int index) throws NTLMEngineException {
        if (src.length < index + 4)
            throw new NTLMEngineException("NTLM authentication - buffer too small for DWORD");
        return (src[index] & 0xff) | ((src[index + 1] & 0xff) << 8)
                | ((src[index + 2] & 0xff) << 16) | ((src[index + 3] & 0xff) << 24);
    }

    private static int readUShort(byte[] src, int index) throws NTLMEngineException {
        if (src.length < index + 2)
            throw new NTLMEngineException("NTLM authentication - buffer too small for WORD");
        return (src[index] & 0xff) | ((src[index + 1] & 0xff) << 8);
    }

    private static byte[] readSecurityBuffer(byte[] src, int index) throws NTLMEngineException {
        int length = readUShort(src, index);
        int offset = readULong(src, index + 4);
        if (src.length < offset + length)
            throw new NTLMEngineException(
                    "NTLM authentication - buffer too small for data item");
        byte[] buffer = new byte[length];
        System.arraycopy(src, offset, buffer, 0, length);
        return buffer;
    }

    
    private static byte[] makeRandomChallenge() throws NTLMEngineException {
        if (RND_GEN == null) {
            throw new NTLMEngineException("Random generator not available");
        }
        byte[] rval = new byte[8];
        synchronized (RND_GEN) {
            RND_GEN.nextBytes(rval);
        }
        return rval;
    }

    
    private static byte[] makeNTLM2RandomChallenge() throws NTLMEngineException {
        if (RND_GEN == null) {
            throw new NTLMEngineException("Random generator not available");
        }
        byte[] rval = new byte[24];
        synchronized (RND_GEN) {
            RND_GEN.nextBytes(rval);
        }
        
        Arrays.fill(rval, 8, 24, (byte) 0x00);
        return rval;
    }

    










    static byte[] getLMResponse(String password, byte[] challenge)
            throws NTLMEngineException {
        byte[] lmHash = lmHash(password);
        return lmResponse(lmHash, challenge);
    }

    










    static byte[] getNTLMResponse(String password, byte[] challenge)
            throws NTLMEngineException {
        byte[] ntlmHash = ntlmHash(password);
        return lmResponse(ntlmHash, challenge);
    }

    



















    static byte[] getNTLMv2Response(String target, String user, String password,
            byte[] challenge, byte[] clientChallenge, byte[] targetInformation)
            throws NTLMEngineException {
        byte[] ntlmv2Hash = ntlmv2Hash(target, user, password);
        byte[] blob = createBlob(clientChallenge, targetInformation);
        return lmv2Response(ntlmv2Hash, challenge, blob);
    }

    
















    static byte[] getLMv2Response(String target, String user, String password,
            byte[] challenge, byte[] clientChallenge) throws NTLMEngineException {
        byte[] ntlmv2Hash = ntlmv2Hash(target, user, password);
        return lmv2Response(ntlmv2Hash, challenge, clientChallenge);
    }

    














    static byte[] getNTLM2SessionResponse(String password, byte[] challenge,
            byte[] clientChallenge) throws NTLMEngineException {
        try {
            byte[] ntlmHash = ntlmHash(password);

            
            
            
            
            
            
            
            
            
            
            
            

            MessageDigest md5 = MessageDigest.getInstance("MD5");
            md5.update(challenge);
            md5.update(clientChallenge);
            byte[] digest = md5.digest();

            byte[] sessionHash = new byte[8];
            System.arraycopy(digest, 0, sessionHash, 0, 8);
            return lmResponse(ntlmHash, sessionHash);
        } catch (Exception e) {
            if (e instanceof NTLMEngineException)
                throw (NTLMEngineException) e;
            throw new NTLMEngineException(e.getMessage(), e);
        }
    }

    








    private static byte[] lmHash(String password) throws NTLMEngineException {
        try {
            byte[] oemPassword = password.toUpperCase().getBytes("US-ASCII");
            int length = Math.min(oemPassword.length, 14);
            byte[] keyBytes = new byte[14];
            System.arraycopy(oemPassword, 0, keyBytes, 0, length);
            Key lowKey = createDESKey(keyBytes, 0);
            Key highKey = createDESKey(keyBytes, 7);
            byte[] magicConstant = "KGS!@#$%".getBytes("US-ASCII");
            Cipher des = Cipher.getInstance("DES/ECB/NoPadding");
            des.init(Cipher.ENCRYPT_MODE, lowKey);
            byte[] lowHash = des.doFinal(magicConstant);
            des.init(Cipher.ENCRYPT_MODE, highKey);
            byte[] highHash = des.doFinal(magicConstant);
            byte[] lmHash = new byte[16];
            System.arraycopy(lowHash, 0, lmHash, 0, 8);
            System.arraycopy(highHash, 0, lmHash, 8, 8);
            return lmHash;
        } catch (Exception e) {
            throw new NTLMEngineException(e.getMessage(), e);
        }
    }

    








    private static byte[] ntlmHash(String password) throws NTLMEngineException {
        try {
            byte[] unicodePassword = password.getBytes("UnicodeLittleUnmarked");
            MD4 md4 = new MD4();
            md4.update(unicodePassword);
            return md4.getOutput();
        } catch (java.io.UnsupportedEncodingException e) {
            throw new NTLMEngineException("Unicode not supported: " + e.getMessage(), e);
        }
    }

    












    private static byte[] ntlmv2Hash(String target, String user, String password)
            throws NTLMEngineException {
        try {
            byte[] ntlmHash = ntlmHash(password);
            HMACMD5 hmacMD5 = new HMACMD5(ntlmHash);
            
            hmacMD5.update(user.toUpperCase().getBytes("UnicodeLittleUnmarked"));
            hmacMD5.update(target.getBytes("UnicodeLittleUnmarked"));
            return hmacMD5.getOutput();
        } catch (java.io.UnsupportedEncodingException e) {
            throw new NTLMEngineException("Unicode not supported! " + e.getMessage(), e);
        }
    }

    









    private static byte[] lmResponse(byte[] hash, byte[] challenge) throws NTLMEngineException {
        try {
            byte[] keyBytes = new byte[21];
            System.arraycopy(hash, 0, keyBytes, 0, 16);
            Key lowKey = createDESKey(keyBytes, 0);
            Key middleKey = createDESKey(keyBytes, 7);
            Key highKey = createDESKey(keyBytes, 14);
            Cipher des = Cipher.getInstance("DES/ECB/NoPadding");
            des.init(Cipher.ENCRYPT_MODE, lowKey);
            byte[] lowResponse = des.doFinal(challenge);
            des.init(Cipher.ENCRYPT_MODE, middleKey);
            byte[] middleResponse = des.doFinal(challenge);
            des.init(Cipher.ENCRYPT_MODE, highKey);
            byte[] highResponse = des.doFinal(challenge);
            byte[] lmResponse = new byte[24];
            System.arraycopy(lowResponse, 0, lmResponse, 0, 8);
            System.arraycopy(middleResponse, 0, lmResponse, 8, 8);
            System.arraycopy(highResponse, 0, lmResponse, 16, 8);
            return lmResponse;
        } catch (Exception e) {
            throw new NTLMEngineException(e.getMessage(), e);
        }
    }

    













    private static byte[] lmv2Response(byte[] hash, byte[] challenge, byte[] clientData)
            throws NTLMEngineException {
        HMACMD5 hmacMD5 = new HMACMD5(hash);
        hmacMD5.update(challenge);
        hmacMD5.update(clientData);
        byte[] mac = hmacMD5.getOutput();
        byte[] lmv2Response = new byte[mac.length + clientData.length];
        System.arraycopy(mac, 0, lmv2Response, 0, mac.length);
        System.arraycopy(clientData, 0, lmv2Response, mac.length, clientData.length);
        return lmv2Response;
    }

    










    private static byte[] createBlob(byte[] clientChallenge, byte[] targetInformation) {
        byte[] blobSignature = new byte[] { (byte) 0x01, (byte) 0x01, (byte) 0x00, (byte) 0x00 };
        byte[] reserved = new byte[] { (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00 };
        byte[] unknown1 = new byte[] { (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00 };
        long time = System.currentTimeMillis();
        time += 11644473600000l; 
        time *= 10000; 
        
        byte[] timestamp = new byte[8];
        for (int i = 0; i < 8; i++) {
            timestamp[i] = (byte) time;
            time >>>= 8;
        }
        byte[] blob = new byte[blobSignature.length + reserved.length + timestamp.length + 8
                + unknown1.length + targetInformation.length];
        int offset = 0;
        System.arraycopy(blobSignature, 0, blob, offset, blobSignature.length);
        offset += blobSignature.length;
        System.arraycopy(reserved, 0, blob, offset, reserved.length);
        offset += reserved.length;
        System.arraycopy(timestamp, 0, blob, offset, timestamp.length);
        offset += timestamp.length;
        System.arraycopy(clientChallenge, 0, blob, offset, 8);
        offset += 8;
        System.arraycopy(unknown1, 0, blob, offset, unknown1.length);
        offset += unknown1.length;
        System.arraycopy(targetInformation, 0, blob, offset, targetInformation.length);
        return blob;
    }

    











    private static Key createDESKey(byte[] bytes, int offset) {
        byte[] keyBytes = new byte[7];
        System.arraycopy(bytes, offset, keyBytes, 0, 7);
        byte[] material = new byte[8];
        material[0] = keyBytes[0];
        material[1] = (byte) (keyBytes[0] << 7 | (keyBytes[1] & 0xff) >>> 1);
        material[2] = (byte) (keyBytes[1] << 6 | (keyBytes[2] & 0xff) >>> 2);
        material[3] = (byte) (keyBytes[2] << 5 | (keyBytes[3] & 0xff) >>> 3);
        material[4] = (byte) (keyBytes[3] << 4 | (keyBytes[4] & 0xff) >>> 4);
        material[5] = (byte) (keyBytes[4] << 3 | (keyBytes[5] & 0xff) >>> 5);
        material[6] = (byte) (keyBytes[5] << 2 | (keyBytes[6] & 0xff) >>> 6);
        material[7] = (byte) (keyBytes[6] << 1);
        oddParity(material);
        return new SecretKeySpec(material, "DES");
    }

    





    private static void oddParity(byte[] bytes) {
        for (int i = 0; i < bytes.length; i++) {
            byte b = bytes[i];
            boolean needsParity = (((b >>> 7) ^ (b >>> 6) ^ (b >>> 5) ^ (b >>> 4) ^ (b >>> 3)
                    ^ (b >>> 2) ^ (b >>> 1)) & 0x01) == 0;
            if (needsParity) {
                bytes[i] |= (byte) 0x01;
            } else {
                bytes[i] &= (byte) 0xfe;
            }
        }
    }

    
    static class NTLMMessage {
        
        private byte[] messageContents = null;

        
        private int currentOutputPosition = 0;

        
        NTLMMessage() {
        }

        
        NTLMMessage(String messageBody, int expectedType) throws NTLMEngineException {
            messageContents = Base64.decodeBase64(EncodingUtils.getBytes(messageBody,
                    DEFAULT_CHARSET));
            
            if (messageContents.length < SIGNATURE.length)
                throw new NTLMEngineException("NTLM message decoding error - packet too short");
            int i = 0;
            while (i < SIGNATURE.length) {
                if (messageContents[i] != SIGNATURE[i])
                    throw new NTLMEngineException(
                            "NTLM message expected - instead got unrecognized bytes");
                i++;
            }

            
            int type = readULong(SIGNATURE.length);
            if (type != expectedType)
                throw new NTLMEngineException("NTLM type " + Integer.toString(expectedType)
                        + " message expected - instead got type " + Integer.toString(type));

            currentOutputPosition = messageContents.length;
        }

        



        protected int getPreambleLength() {
            return SIGNATURE.length + 4;
        }

        
        protected int getMessageLength() {
            return currentOutputPosition;
        }

        
        protected byte readByte(int position) throws NTLMEngineException {
            if (messageContents.length < position + 1)
                throw new NTLMEngineException("NTLM: Message too short");
            return messageContents[position];
        }

        
        protected void readBytes(byte[] buffer, int position) throws NTLMEngineException {
            if (messageContents.length < position + buffer.length)
                throw new NTLMEngineException("NTLM: Message too short");
            System.arraycopy(messageContents, position, buffer, 0, buffer.length);
        }

        
        protected int readUShort(int position) throws NTLMEngineException {
            return NTLMEngineImpl.readUShort(messageContents, position);
        }

        
        protected int readULong(int position) throws NTLMEngineException {
            return NTLMEngineImpl.readULong(messageContents, position);
        }

        
        protected byte[] readSecurityBuffer(int position) throws NTLMEngineException {
            return NTLMEngineImpl.readSecurityBuffer(messageContents, position);
        }

        







        protected void prepareResponse(int maxlength, int messageType) {
            messageContents = new byte[maxlength];
            currentOutputPosition = 0;
            addBytes(SIGNATURE);
            addULong(messageType);
        }

        





        protected void addByte(byte b) {
            messageContents[currentOutputPosition] = b;
            currentOutputPosition++;
        }

        





        protected void addBytes(byte[] bytes) {
            for (int i = 0; i < bytes.length; i++) {
                messageContents[currentOutputPosition] = bytes[i];
                currentOutputPosition++;
            }
        }

        
        protected void addUShort(int value) {
            addByte((byte) (value & 0xff));
            addByte((byte) (value >> 8 & 0xff));
        }

        
        protected void addULong(int value) {
            addByte((byte) (value & 0xff));
            addByte((byte) (value >> 8 & 0xff));
            addByte((byte) (value >> 16 & 0xff));
            addByte((byte) (value >> 24 & 0xff));
        }

        





        String getResponse() {
            byte[] resp;
            if (messageContents.length > currentOutputPosition) {
                byte[] tmp = new byte[currentOutputPosition];
                for (int i = 0; i < currentOutputPosition; i++) {
                    tmp[i] = messageContents[i];
                }
                resp = tmp;
            } else {
                resp = messageContents;
            }
            return EncodingUtils.getAsciiString(Base64.encodeBase64(resp));
        }

    }

    
    static class Type1Message extends NTLMMessage {
        protected byte[] hostBytes;
        protected byte[] domainBytes;

        
        Type1Message(String domain, String host) throws NTLMEngineException {
            super();
            try {
                
                host = convertHost(host);
                
                domain = convertDomain(domain);

                hostBytes = host.getBytes("UnicodeLittleUnmarked");
                domainBytes = domain.toUpperCase().getBytes("UnicodeLittleUnmarked");
            } catch (java.io.UnsupportedEncodingException e) {
                throw new NTLMEngineException("Unicode unsupported: " + e.getMessage(), e);
            }
        }

        



        @Override
        String getResponse() {
            
            
            int finalLength = 32 + hostBytes.length + domainBytes.length;

            
            
            prepareResponse(finalLength, 1);

            
            addULong(FLAG_NEGOTIATE_NTLM | FLAG_NEGOTIATE_NTLM2 | FLAG_NEGOTIATE_SIGN
                    | FLAG_NEGOTIATE_SEAL |
                    


                    FLAG_UNICODE_ENCODING | FLAG_TARGET_DESIRED | FLAG_NEGOTIATE_128);

            
            addUShort(domainBytes.length);
            addUShort(domainBytes.length);

            
            addULong(hostBytes.length + 32);

            
            addUShort(hostBytes.length);
            addUShort(hostBytes.length);

            
            addULong(32);

            
            addBytes(hostBytes);

            
            addBytes(domainBytes);

            return super.getResponse();
        }

    }

    
    static class Type2Message extends NTLMMessage {
        protected byte[] challenge;
        protected String target;
        protected byte[] targetInfo;
        protected int flags;

        Type2Message(String message) throws NTLMEngineException {
            super(message, 2);

            
            
            challenge = new byte[8];
            readBytes(challenge, 24);

            flags = readULong(20);
            if ((flags & FLAG_UNICODE_ENCODING) == 0)
                throw new NTLMEngineException(
                        "NTLM type 2 message has flags that make no sense: "
                                + Integer.toString(flags));
            
            target = null;
            
            
            
            if (getMessageLength() >= 12 + 8) {
                byte[] bytes = readSecurityBuffer(12);
                if (bytes.length != 0) {
                    try {
                        target = new String(bytes, "UnicodeLittleUnmarked");
                    } catch (java.io.UnsupportedEncodingException e) {
                        throw new NTLMEngineException(e.getMessage(), e);
                    }
                }
            }

            
            targetInfo = null;
            
            if (getMessageLength() >= 40 + 8) {
                byte[] bytes = readSecurityBuffer(40);
                if (bytes.length != 0) {
                    targetInfo = bytes;
                }
            }
        }

        
        byte[] getChallenge() {
            return challenge;
        }

        
        String getTarget() {
            return target;
        }

        
        byte[] getTargetInfo() {
            return targetInfo;
        }

        
        int getFlags() {
            return flags;
        }

    }

    
    static class Type3Message extends NTLMMessage {
        
        protected int type2Flags;

        protected byte[] domainBytes;
        protected byte[] hostBytes;
        protected byte[] userBytes;

        protected byte[] lmResp;
        protected byte[] ntResp;

        
        Type3Message(String domain, String host, String user, String password, byte[] nonce,
                int type2Flags, String target, byte[] targetInformation)
                throws NTLMEngineException {
            
            this.type2Flags = type2Flags;

            
            host = convertHost(host);
            
            domain = convertDomain(domain);

            
            
            try {
                if (targetInformation != null && target != null) {
                    byte[] clientChallenge = makeRandomChallenge();
                    ntResp = getNTLMv2Response(target, user, password, nonce, clientChallenge,
                            targetInformation);
                    lmResp = getLMv2Response(target, user, password, nonce, clientChallenge);
                } else {
                    if ((type2Flags & FLAG_NEGOTIATE_NTLM2) != 0) {
                        
                        byte[] clientChallenge = makeNTLM2RandomChallenge();

                        ntResp = getNTLM2SessionResponse(password, nonce, clientChallenge);
                        lmResp = clientChallenge;

                        
                        
                        
                        
                    } else {
                        ntResp = getNTLMResponse(password, nonce);
                        lmResp = getLMResponse(password, nonce);
                    }
                }
            } catch (NTLMEngineException e) {
                
                
                ntResp = new byte[0];
                lmResp = getLMResponse(password, nonce);
            }

            try {
                domainBytes = domain.toUpperCase().getBytes("UnicodeLittleUnmarked");
                hostBytes = host.getBytes("UnicodeLittleUnmarked");
                userBytes = user.getBytes("UnicodeLittleUnmarked");
            } catch (java.io.UnsupportedEncodingException e) {
                throw new NTLMEngineException("Unicode not supported: " + e.getMessage(), e);
            }
        }

        
        @Override
        String getResponse() {
            int ntRespLen = ntResp.length;
            int lmRespLen = lmResp.length;

            int domainLen = domainBytes.length;
            int hostLen = hostBytes.length;
            int userLen = userBytes.length;

            
            int lmRespOffset = 64;
            int ntRespOffset = lmRespOffset + lmRespLen;
            int domainOffset = ntRespOffset + ntRespLen;
            int userOffset = domainOffset + domainLen;
            int hostOffset = userOffset + userLen;
            int sessionKeyOffset = hostOffset + hostLen;
            int finalLength = sessionKeyOffset + 0;

            
            prepareResponse(finalLength, 3);

            
            addUShort(lmRespLen);
            addUShort(lmRespLen);

            
            addULong(lmRespOffset);

            
            addUShort(ntRespLen);
            addUShort(ntRespLen);

            
            addULong(ntRespOffset);

            
            addUShort(domainLen);
            addUShort(domainLen);

            
            addULong(domainOffset);

            
            addUShort(userLen);
            addUShort(userLen);

            
            addULong(userOffset);

            
            addUShort(hostLen);
            addUShort(hostLen);

            
            addULong(hostOffset);

            
            addULong(0);

            
            addULong(finalLength);

            
            
            addULong(FLAG_NEGOTIATE_NTLM | FLAG_UNICODE_ENCODING | FLAG_TARGET_DESIRED
                    | FLAG_NEGOTIATE_128 | (type2Flags & FLAG_NEGOTIATE_NTLM2)
                    | (type2Flags & FLAG_NEGOTIATE_SIGN) | (type2Flags & FLAG_NEGOTIATE_SEAL)
                    | (type2Flags & FLAG_NEGOTIATE_KEY_EXCH)
                    | (type2Flags & FLAG_NEGOTIATE_ALWAYS_SIGN));

            
            addBytes(lmResp);
            addBytes(ntResp);
            addBytes(domainBytes);
            addBytes(userBytes);
            addBytes(hostBytes);

            return super.getResponse();
        }
    }

    static void writeULong(byte[] buffer, int value, int offset) {
        buffer[offset] = (byte) (value & 0xff);
        buffer[offset + 1] = (byte) (value >> 8 & 0xff);
        buffer[offset + 2] = (byte) (value >> 16 & 0xff);
        buffer[offset + 3] = (byte) (value >> 24 & 0xff);
    }

    static int F(int x, int y, int z) {
        return ((x & y) | (~x & z));
    }

    static int G(int x, int y, int z) {
        return ((x & y) | (x & z) | (y & z));
    }

    static int H(int x, int y, int z) {
        return (x ^ y ^ z);
    }

    static int rotintlft(int val, int numbits) {
        return ((val << numbits) | (val >>> (32 - numbits)));
    }

    






    static class MD4 {
        protected int A = 0x67452301;
        protected int B = 0xefcdab89;
        protected int C = 0x98badcfe;
        protected int D = 0x10325476;
        protected long count = 0L;
        protected byte[] dataBuffer = new byte[64];

        MD4() {
        }

        void update(byte[] input) {
            
            
            
            int curBufferPos = (int) (count & 63L);
            int inputIndex = 0;
            while (input.length - inputIndex + curBufferPos >= dataBuffer.length) {
                
                
                
                int transferAmt = dataBuffer.length - curBufferPos;
                System.arraycopy(input, inputIndex, dataBuffer, curBufferPos, transferAmt);
                count += transferAmt;
                curBufferPos = 0;
                inputIndex += transferAmt;
                processBuffer();
            }

            
            
            if (inputIndex < input.length) {
                int transferAmt = input.length - inputIndex;
                System.arraycopy(input, inputIndex, dataBuffer, curBufferPos, transferAmt);
                count += transferAmt;
                curBufferPos += transferAmt;
            }
        }

        byte[] getOutput() {
            
            
            int bufferIndex = (int) (count & 63L);
            int padLen = (bufferIndex < 56) ? (56 - bufferIndex) : (120 - bufferIndex);
            byte[] postBytes = new byte[padLen + 8];
            
            
            postBytes[0] = (byte) 0x80;
            
            for (int i = 0; i < 8; i++) {
                postBytes[padLen + i] = (byte) ((count * 8) >>> (8 * i));
            }

            
            update(postBytes);

            
            byte[] result = new byte[16];
            writeULong(result, A, 0);
            writeULong(result, B, 4);
            writeULong(result, C, 8);
            writeULong(result, D, 12);
            return result;
        }

        protected void processBuffer() {
            
            int[] d = new int[16];

            for (int i = 0; i < 16; i++) {
                d[i] = (dataBuffer[i * 4] & 0xff) + ((dataBuffer[i * 4 + 1] & 0xff) << 8)
                        + ((dataBuffer[i * 4 + 2] & 0xff) << 16)
                        + ((dataBuffer[i * 4 + 3] & 0xff) << 24);
            }

            
            int AA = A;
            int BB = B;
            int CC = C;
            int DD = D;
            round1(d);
            round2(d);
            round3(d);
            A += AA;
            B += BB;
            C += CC;
            D += DD;

        }

        protected void round1(int[] d) {
            A = rotintlft((A + F(B, C, D) + d[0]), 3);
            D = rotintlft((D + F(A, B, C) + d[1]), 7);
            C = rotintlft((C + F(D, A, B) + d[2]), 11);
            B = rotintlft((B + F(C, D, A) + d[3]), 19);

            A = rotintlft((A + F(B, C, D) + d[4]), 3);
            D = rotintlft((D + F(A, B, C) + d[5]), 7);
            C = rotintlft((C + F(D, A, B) + d[6]), 11);
            B = rotintlft((B + F(C, D, A) + d[7]), 19);

            A = rotintlft((A + F(B, C, D) + d[8]), 3);
            D = rotintlft((D + F(A, B, C) + d[9]), 7);
            C = rotintlft((C + F(D, A, B) + d[10]), 11);
            B = rotintlft((B + F(C, D, A) + d[11]), 19);

            A = rotintlft((A + F(B, C, D) + d[12]), 3);
            D = rotintlft((D + F(A, B, C) + d[13]), 7);
            C = rotintlft((C + F(D, A, B) + d[14]), 11);
            B = rotintlft((B + F(C, D, A) + d[15]), 19);
        }

        protected void round2(int[] d) {
            A = rotintlft((A + G(B, C, D) + d[0] + 0x5a827999), 3);
            D = rotintlft((D + G(A, B, C) + d[4] + 0x5a827999), 5);
            C = rotintlft((C + G(D, A, B) + d[8] + 0x5a827999), 9);
            B = rotintlft((B + G(C, D, A) + d[12] + 0x5a827999), 13);

            A = rotintlft((A + G(B, C, D) + d[1] + 0x5a827999), 3);
            D = rotintlft((D + G(A, B, C) + d[5] + 0x5a827999), 5);
            C = rotintlft((C + G(D, A, B) + d[9] + 0x5a827999), 9);
            B = rotintlft((B + G(C, D, A) + d[13] + 0x5a827999), 13);

            A = rotintlft((A + G(B, C, D) + d[2] + 0x5a827999), 3);
            D = rotintlft((D + G(A, B, C) + d[6] + 0x5a827999), 5);
            C = rotintlft((C + G(D, A, B) + d[10] + 0x5a827999), 9);
            B = rotintlft((B + G(C, D, A) + d[14] + 0x5a827999), 13);

            A = rotintlft((A + G(B, C, D) + d[3] + 0x5a827999), 3);
            D = rotintlft((D + G(A, B, C) + d[7] + 0x5a827999), 5);
            C = rotintlft((C + G(D, A, B) + d[11] + 0x5a827999), 9);
            B = rotintlft((B + G(C, D, A) + d[15] + 0x5a827999), 13);

        }

        protected void round3(int[] d) {
            A = rotintlft((A + H(B, C, D) + d[0] + 0x6ed9eba1), 3);
            D = rotintlft((D + H(A, B, C) + d[8] + 0x6ed9eba1), 9);
            C = rotintlft((C + H(D, A, B) + d[4] + 0x6ed9eba1), 11);
            B = rotintlft((B + H(C, D, A) + d[12] + 0x6ed9eba1), 15);

            A = rotintlft((A + H(B, C, D) + d[2] + 0x6ed9eba1), 3);
            D = rotintlft((D + H(A, B, C) + d[10] + 0x6ed9eba1), 9);
            C = rotintlft((C + H(D, A, B) + d[6] + 0x6ed9eba1), 11);
            B = rotintlft((B + H(C, D, A) + d[14] + 0x6ed9eba1), 15);

            A = rotintlft((A + H(B, C, D) + d[1] + 0x6ed9eba1), 3);
            D = rotintlft((D + H(A, B, C) + d[9] + 0x6ed9eba1), 9);
            C = rotintlft((C + H(D, A, B) + d[5] + 0x6ed9eba1), 11);
            B = rotintlft((B + H(C, D, A) + d[13] + 0x6ed9eba1), 15);

            A = rotintlft((A + H(B, C, D) + d[3] + 0x6ed9eba1), 3);
            D = rotintlft((D + H(A, B, C) + d[11] + 0x6ed9eba1), 9);
            C = rotintlft((C + H(D, A, B) + d[7] + 0x6ed9eba1), 11);
            B = rotintlft((B + H(C, D, A) + d[15] + 0x6ed9eba1), 15);

        }

    }

    



    static class HMACMD5 {
        protected byte[] ipad;
        protected byte[] opad;
        protected MessageDigest md5;

        HMACMD5(byte[] key) throws NTLMEngineException {
            try {
                md5 = MessageDigest.getInstance("MD5");
            } catch (Exception ex) {
                
                
                throw new NTLMEngineException(
                        "Error getting md5 message digest implementation: " + ex.getMessage(), ex);
            }

            
            ipad = new byte[64];
            opad = new byte[64];

            int keyLength = key.length;
            if (keyLength > 64) {
                
                md5.update(key);
                key = md5.digest();
                keyLength = key.length;
            }
            int i = 0;
            while (i < keyLength) {
                ipad[i] = (byte) (key[i] ^ (byte) 0x36);
                opad[i] = (byte) (key[i] ^ (byte) 0x5c);
                i++;
            }
            while (i < 64) {
                ipad[i] = (byte) 0x36;
                opad[i] = (byte) 0x5c;
                i++;
            }

            
            md5.reset();
            md5.update(ipad);

        }

        
        byte[] getOutput() {
            byte[] digest = md5.digest();
            md5.update(opad);
            return md5.digest(digest);
        }

        
        void update(byte[] input) {
            md5.update(input);
        }

        
        void update(byte[] input, int offset, int length) {
            md5.update(input, offset, length);
        }

    }

    public String generateType1Msg(
            final String domain,
            final String workstation) throws NTLMEngineException {
        return getType1Message(workstation, domain);
    }

    public String generateType3Msg(
            final String username,
            final String password,
            final String domain,
            final String workstation,
            final String challenge) throws NTLMEngineException {
        Type2Message t2m = new Type2Message(challenge);
        return getType3Message(
                username,
                password,
                workstation,
                domain,
                t2m.getChallenge(),
                t2m.getFlags(),
                t2m.getTarget(),
                t2m.getTargetInfo());
    }

}
