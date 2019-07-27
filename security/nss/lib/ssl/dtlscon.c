







#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"

#ifndef PR_ARRAY_SIZE
#define PR_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

static SECStatus dtls_TransmitMessageFlight(sslSocket *ss);
static void dtls_RetransmitTimerExpiredCb(sslSocket *ss);
static SECStatus dtls_SendSavedWriteData(sslSocket *ss);


static const PRUint16 COMMON_MTU_VALUES[] = {
    1500 - 28,  
    1280 - 28,  
    576 - 28,   
    256 - 28    
};

#define DTLS_COOKIE_BYTES 32


static const ssl3CipherSuite nonDTLSSuites[] = {
#ifndef NSS_DISABLE_ECC
    TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,
    TLS_ECDHE_RSA_WITH_RC4_128_SHA,
#endif 
    TLS_DHE_DSS_WITH_RC4_128_SHA,
#ifndef NSS_DISABLE_ECC
    TLS_ECDH_RSA_WITH_RC4_128_SHA,
    TLS_ECDH_ECDSA_WITH_RC4_128_SHA,
#endif 
    TLS_RSA_WITH_RC4_128_MD5,
    TLS_RSA_WITH_RC4_128_SHA,
    TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,
    TLS_RSA_EXPORT_WITH_RC4_40_MD5,
    0 
};









SSL3ProtocolVersion
dtls_TLSVersionToDTLSVersion(SSL3ProtocolVersion tlsv)
{
    if (tlsv == SSL_LIBRARY_VERSION_TLS_1_1) {
        return SSL_LIBRARY_VERSION_DTLS_1_0_WIRE;
    }
    if (tlsv == SSL_LIBRARY_VERSION_TLS_1_2) {
        return SSL_LIBRARY_VERSION_DTLS_1_2_WIRE;
    }
    if (tlsv == SSL_LIBRARY_VERSION_TLS_1_3) {
        return SSL_LIBRARY_VERSION_DTLS_1_3_WIRE;
    }

    

    return 0xffff;
}





SSL3ProtocolVersion
dtls_DTLSVersionToTLSVersion(SSL3ProtocolVersion dtlsv)
{
    if (MSB(dtlsv) == 0xff) {
        return 0;
    }

    if (dtlsv == SSL_LIBRARY_VERSION_DTLS_1_0_WIRE) {
        return SSL_LIBRARY_VERSION_TLS_1_1;
    }
    if (dtlsv == SSL_LIBRARY_VERSION_DTLS_1_2_WIRE) {
        return SSL_LIBRARY_VERSION_TLS_1_2;
    }
    if (dtlsv == SSL_LIBRARY_VERSION_DTLS_1_3_WIRE) {
        return SSL_LIBRARY_VERSION_TLS_1_3;
    }

    
    return SSL_LIBRARY_VERSION_TLS_1_2 + 1;
}


SECStatus
ssl3_DisableNonDTLSSuites(sslSocket * ss)
{
    const ssl3CipherSuite * suite;

    for (suite = nonDTLSSuites; *suite; ++suite) {
        SECStatus rv = ssl3_CipherPrefSet(ss, *suite, PR_FALSE);

        PORT_Assert(rv == SECSuccess); 
    }
    return SECSuccess;
}





static DTLSQueuedMessage *
dtls_AllocQueuedMessage(PRUint16 epoch, SSL3ContentType type,
                        const unsigned char *data, PRUint32 len)
{
    DTLSQueuedMessage *msg = NULL;

    msg = PORT_ZAlloc(sizeof(DTLSQueuedMessage));
    if (!msg)
        return NULL;

    msg->data = PORT_Alloc(len);
    if (!msg->data) {
        PORT_Free(msg);
        return NULL;
    }
    PORT_Memcpy(msg->data, data, len);

    msg->len = len;
    msg->epoch = epoch;
    msg->type = type;

    return msg;
}






static void
dtls_FreeHandshakeMessage(DTLSQueuedMessage *msg)
{
    if (!msg)
        return;

    PORT_ZFree(msg->data, msg->len);
    PORT_Free(msg);
}








void
dtls_FreeHandshakeMessages(PRCList *list)
{
    PRCList *cur_p;

    while (!PR_CLIST_IS_EMPTY(list)) {
        cur_p = PR_LIST_TAIL(list);
        PR_REMOVE_LINK(cur_p);
        dtls_FreeHandshakeMessage((DTLSQueuedMessage *)cur_p);
    }
}















#define OFFSET_BYTE(o) (o/8)
#define OFFSET_MASK(o) (1 << (o%8))

SECStatus
dtls_HandleHandshake(sslSocket *ss, sslBuffer *origBuf)
{
    





    sslBuffer buf = *origBuf;
    SECStatus rv = SECSuccess;

    PORT_Assert(ss->opt.noLocks || ssl_HaveRecvBufLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    while (buf.len > 0) {
        PRUint8 type;
        PRUint32 message_length;
        PRUint16 message_seq;
        PRUint32 fragment_offset;
        PRUint32 fragment_length;
        PRUint32 offset;

        if (buf.len < 12) {
            PORT_SetError(SSL_ERROR_RX_MALFORMED_HANDSHAKE);
            rv = SECFailure;
            break;
        }

        
        type = buf.buf[0];
        message_length = (buf.buf[1] << 16) | (buf.buf[2] << 8) | buf.buf[3];
        message_seq = (buf.buf[4] << 8) | buf.buf[5];
        fragment_offset = (buf.buf[6] << 16) | (buf.buf[7] << 8) | buf.buf[8];
        fragment_length = (buf.buf[9] << 16) | (buf.buf[10] << 8) | buf.buf[11];

#define MAX_HANDSHAKE_MSG_LEN 0x1ffff   /* 128k - 1 */
        if (message_length > MAX_HANDSHAKE_MSG_LEN) {
            (void)ssl3_DecodeError(ss);
            PORT_SetError(SSL_ERROR_RX_MALFORMED_HANDSHAKE);
            return SECFailure;
        }
#undef MAX_HANDSHAKE_MSG_LEN

        buf.buf += 12;
        buf.len -= 12;

        
        if (buf.len < fragment_length) {
            PORT_SetError(SSL_ERROR_RX_MALFORMED_HANDSHAKE);
            rv = SECFailure;
            break;
        }

        
        if ((fragment_length + fragment_offset) > message_length) {
            PORT_SetError(SSL_ERROR_RX_MALFORMED_HANDSHAKE);
            rv = SECFailure;
            break;
        }

        








        if ((message_seq == ss->ssl3.hs.recvMessageSeq)
            && (fragment_offset == 0)
            && (fragment_length == message_length)) {
            
            ss->ssl3.hs.msg_type = (SSL3HandshakeType)type;
            ss->ssl3.hs.msg_len = message_length;

            

            dtls_FreeHandshakeMessages(&ss->ssl3.hs.lastMessageFlight);
            ss->ssl3.hs.recvdHighWater = -1;
            dtls_CancelTimer(ss);

            

            if (ss->ssl3.hs.rtRetries == 0) {
                ss->ssl3.hs.rtTimeoutMs = INITIAL_DTLS_TIMEOUT_MS;
            }

            rv = ssl3_HandleHandshakeMessage(ss, buf.buf, ss->ssl3.hs.msg_len);
            if (rv == SECFailure) {
                
                break;
            }
        } else {
            if (message_seq < ss->ssl3.hs.recvMessageSeq) {
                

                if (ss->ssl3.hs.rtTimerCb == NULL) {
                    
                } else if (ss->ssl3.hs.rtTimerCb ==
                         dtls_RetransmitTimerExpiredCb) {
                    SSL_TRC(30, ("%d: SSL3[%d]: Retransmit detected",
                                 SSL_GETPID(), ss->fd));
                    




                    if ((PR_IntervalNow() - ss->ssl3.hs.rtTimerStarted) >
                        (ss->ssl3.hs.rtTimeoutMs / 4)) {
                            SSL_TRC(30,
                            ("%d: SSL3[%d]: Shortcutting retransmit timer",
                            SSL_GETPID(), ss->fd));

                            

                            dtls_CancelTimer(ss);
                            dtls_RetransmitTimerExpiredCb(ss);
                            rv = SECSuccess;
                            break;
                        } else {
                            SSL_TRC(30,
                            ("%d: SSL3[%d]: We just retransmitted. Ignoring.",
                            SSL_GETPID(), ss->fd));
                            rv = SECSuccess;
                            break;
                        }
                } else if (ss->ssl3.hs.rtTimerCb == dtls_FinishedTimerCb) {
                    




                    dtls_CancelTimer(ss);
                    rv = dtls_TransmitMessageFlight(ss);
                    if (rv == SECSuccess) {
                        rv = dtls_StartTimer(ss, dtls_FinishedTimerCb);
                    }
                    if (rv != SECSuccess)
                        return rv;
                    break;
                }
            } else if (message_seq > ss->ssl3.hs.recvMessageSeq) {
                







            } else {
                



                
                if (ss->ssl3.hs.recvdHighWater == -1) {
                    PRUint32 map_length = OFFSET_BYTE(message_length) + 1;

                    rv = sslBuffer_Grow(&ss->ssl3.hs.msg_body, message_length);
                    if (rv != SECSuccess)
                        break;
                    
                    rv = sslBuffer_Grow(&ss->ssl3.hs.recvdFragments,
                                        map_length);
                    if (rv != SECSuccess)
                        break;

                    
                    ss->ssl3.hs.recvdHighWater = 0;
                    PORT_Memset(ss->ssl3.hs.recvdFragments.buf, 0,
                                ss->ssl3.hs.recvdFragments.space);
                    ss->ssl3.hs.msg_type = (SSL3HandshakeType)type;
                    ss->ssl3.hs.msg_len = message_length;
                }

                



                if (message_length != ss->ssl3.hs.msg_len) {
                    ss->ssl3.hs.recvdHighWater = -1;
                    PORT_SetError(SSL_ERROR_RX_MALFORMED_HANDSHAKE);
                    rv = SECFailure;
                    break;
                }

                
                PORT_Assert((fragment_offset + fragment_length) <=
                            ss->ssl3.hs.msg_body.space);
                PORT_Memcpy(ss->ssl3.hs.msg_body.buf + fragment_offset,
                            buf.buf, fragment_length);

                










                if (fragment_offset <= ss->ssl3.hs.recvdHighWater) {
                    

                    ss->ssl3.hs.recvdHighWater = fragment_offset +
                                                 fragment_length;
                } else {
                    for (offset = fragment_offset;
                         offset < fragment_offset + fragment_length;
                         offset++) {
                        ss->ssl3.hs.recvdFragments.buf[OFFSET_BYTE(offset)] |=
                            OFFSET_MASK(offset);
                    }
                }

                
                for (offset = ss->ssl3.hs.recvdHighWater;
                     offset < ss->ssl3.hs.msg_len; offset++) {
                    


                    if (ss->ssl3.hs.recvdFragments.buf[OFFSET_BYTE(offset)] &
                        OFFSET_MASK(offset)) {
                        ss->ssl3.hs.recvdHighWater++;
                    } else {
                        break;
                    }
                }

                
                if (ss->ssl3.hs.recvdHighWater == ss->ssl3.hs.msg_len) {
                    ss->ssl3.hs.recvdHighWater = -1;

                    rv = ssl3_HandleHandshakeMessage(ss,
                                                     ss->ssl3.hs.msg_body.buf,
                                                     ss->ssl3.hs.msg_len);
                    if (rv == SECFailure)
                        break; 

                    

                    dtls_FreeHandshakeMessages(&ss->ssl3.hs.lastMessageFlight);
                    dtls_CancelTimer(ss);

                    

                    if (ss->ssl3.hs.rtRetries == 0) {
                        ss->ssl3.hs.rtTimeoutMs = INITIAL_DTLS_TIMEOUT_MS;
                    }
                }
            }
        }

        buf.buf += fragment_length;
        buf.len -= fragment_length;
    }

    origBuf->len = 0;   

    

    return rv;
}







SECStatus dtls_QueueMessage(sslSocket *ss, SSL3ContentType type,
    const SSL3Opaque *pIn, PRInt32 nIn)
{
    SECStatus rv = SECSuccess;
    DTLSQueuedMessage *msg = NULL;

    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    msg = dtls_AllocQueuedMessage(ss->ssl3.cwSpec->epoch, type, pIn, nIn);

    if (!msg) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        rv = SECFailure;
    } else {
        PR_APPEND_LINK(&msg->link, &ss->ssl3.hs.lastMessageFlight);
    }

    return rv;
}










SECStatus
dtls_StageHandshakeMessage(sslSocket *ss)
{
    SECStatus rv = SECSuccess;

    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    

    if (!ss->sec.ci.sendBuf.buf || !ss->sec.ci.sendBuf.len)
        return rv;

    rv = dtls_QueueMessage(ss, content_handshake,
                           ss->sec.ci.sendBuf.buf, ss->sec.ci.sendBuf.len);

    
    ss->sec.ci.sendBuf.len = 0;
    return rv;
}







SECStatus
dtls_FlushHandshakeMessages(sslSocket *ss, PRInt32 flags)
{
    SECStatus rv = SECSuccess;

    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    rv = dtls_StageHandshakeMessage(ss);
    if (rv != SECSuccess)
        return rv;

    if (!(flags & ssl_SEND_FLAG_FORCE_INTO_BUFFER)) {
        rv = dtls_TransmitMessageFlight(ss);
        if (rv != SECSuccess)
            return rv;

        if (!(flags & ssl_SEND_FLAG_NO_RETRANSMIT)) {
            ss->ssl3.hs.rtRetries = 0;
            rv = dtls_StartTimer(ss, dtls_RetransmitTimerExpiredCb);
        }
    }

    return rv;
}







static void
dtls_RetransmitTimerExpiredCb(sslSocket *ss)
{
    SECStatus rv = SECFailure;

    ss->ssl3.hs.rtRetries++;

    if (!(ss->ssl3.hs.rtRetries % 3)) {
        


        dtls_SetMTU(ss, ss->ssl3.hs.maxMessageSent - 1);
    }

    rv = dtls_TransmitMessageFlight(ss);
    if (rv == SECSuccess) {

        
        rv = dtls_RestartTimer(ss, PR_TRUE, dtls_RetransmitTimerExpiredCb);
    }

    if (rv == SECFailure) {
        

    }
}








static SECStatus
dtls_TransmitMessageFlight(sslSocket *ss)
{
    SECStatus rv = SECSuccess;
    PRCList *msg_p;
    PRUint16 room_left = ss->ssl3.mtu;
    PRInt32 sent;

    ssl_GetXmitBufLock(ss);
    ssl_GetSpecReadLock(ss);

    





    PORT_Assert(!ss->pendingBuf.len);
    for (msg_p = PR_LIST_HEAD(&ss->ssl3.hs.lastMessageFlight);
         msg_p != &ss->ssl3.hs.lastMessageFlight;
         msg_p = PR_NEXT_LINK(msg_p)) {
        DTLSQueuedMessage *msg = (DTLSQueuedMessage *)msg_p;

        









        if ((msg->len + SSL3_BUFFER_FUDGE) > room_left) {
            
            rv = dtls_SendSavedWriteData(ss);
            if (rv != SECSuccess)
                break;

            room_left = ss->ssl3.mtu;
        }

        if ((msg->len + SSL3_BUFFER_FUDGE) <= room_left) {
            

            sent = ssl3_SendRecord(ss, msg->epoch, msg->type,
                                   msg->data, msg->len,
                                   ssl_SEND_FLAG_FORCE_INTO_BUFFER |
                                   ssl_SEND_FLAG_USE_EPOCH);
            if (sent != msg->len) {
                rv = SECFailure;
                if (sent != -1) {
                    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
                }
                break;
            }

            room_left = ss->ssl3.mtu - ss->pendingBuf.len;
        } else {
            





            PRUint32 fragment_offset = 0;
            unsigned char fragment[DTLS_MAX_MTU]; 


            
            PORT_Assert(room_left == ss->ssl3.mtu);

            

            PORT_Assert(msg->type == content_handshake);

            


            PORT_Assert(msg->len >= 12);

            while ((fragment_offset + 12) < msg->len) {
                PRUint32 fragment_len;
                const unsigned char *content = msg->data + 12;
                PRUint32 content_len = msg->len - 12;

                

                fragment_len = PR_MIN(room_left - (SSL3_BUFFER_FUDGE + 8),
                                      content_len - fragment_offset);
                PORT_Assert(fragment_len < DTLS_MAX_MTU - 12);
                







                fragment_len = PR_MIN(fragment_len, DTLS_MAX_MTU - 12);

                
                
                PORT_Memcpy(fragment, msg->data, 6);

                
                fragment[6] = (fragment_offset >> 16) & 0xff;
                fragment[7] = (fragment_offset >> 8) & 0xff;
                fragment[8] = (fragment_offset) & 0xff;

                
                fragment[9] = (fragment_len >> 16) & 0xff;
                fragment[10] = (fragment_len >> 8) & 0xff;
                fragment[11] = (fragment_len) & 0xff;

                PORT_Memcpy(fragment + 12, content + fragment_offset,
                            fragment_len);

                



                sent = ssl3_SendRecord(ss, msg->epoch, msg->type,
                                       fragment, fragment_len + 12,
                                       ssl_SEND_FLAG_FORCE_INTO_BUFFER |
                                       ssl_SEND_FLAG_USE_EPOCH);
                if (sent != (fragment_len + 12)) {
                    rv = SECFailure;
                    if (sent != -1) {
                        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
                    }
                    break;
                }

                
                rv = dtls_SendSavedWriteData(ss);
                if (rv != SECSuccess)
                    break;

                fragment_offset += fragment_len;
            }
        }
    }

    
    if (rv == SECSuccess)
        rv = dtls_SendSavedWriteData(ss);

    
    ssl_ReleaseSpecReadLock(ss);
    ssl_ReleaseXmitBufLock(ss);

    return rv;
}







static
SECStatus dtls_SendSavedWriteData(sslSocket *ss)
{
    PRInt32 sent;

    sent = ssl_SendSavedWriteData(ss);
    if (sent < 0)
        return SECFailure;

    

    if (ss->pendingBuf.len > 0) {
        ssl_MapLowLevelError(SSL_ERROR_SOCKET_WRITE_FAILURE);
        return SECFailure;
    }

    

    if (sent > ss->ssl3.hs.maxMessageSent)
        ss->ssl3.hs.maxMessageSent = sent;

    return SECSuccess;
}








SECStatus
dtls_CompressMACEncryptRecord(sslSocket *        ss,
                              DTLSEpoch          epoch,
                              PRBool             use_epoch,
                              SSL3ContentType    type,
                              const SSL3Opaque * pIn,
                              PRUint32           contentLen,
                              sslBuffer        * wrBuf)
{
    SECStatus rv = SECFailure;
    ssl3CipherSpec *          cwSpec;

    ssl_GetSpecReadLock(ss);    

    









    if (use_epoch) {
        if (ss->ssl3.cwSpec->epoch == epoch)
            cwSpec = ss->ssl3.cwSpec;
        else if (ss->ssl3.pwSpec->epoch == epoch)
            cwSpec = ss->ssl3.pwSpec;
        else
            cwSpec = NULL;
    } else {
        cwSpec = ss->ssl3.cwSpec;
    }

    if (cwSpec) {
        rv = ssl3_CompressMACEncryptRecord(cwSpec, ss->sec.isServer, PR_TRUE,
                                           PR_FALSE, type, pIn, contentLen,
                                           wrBuf);
    } else {
        PR_NOT_REACHED("Couldn't find a cipher spec matching epoch");
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
    }
    ssl_ReleaseSpecReadLock(ss); 

    return rv;
}








SECStatus
dtls_StartTimer(sslSocket *ss, DTLSTimerCb cb)
{
    PORT_Assert(ss->ssl3.hs.rtTimerCb == NULL);

    ss->ssl3.hs.rtTimerStarted = PR_IntervalNow();
    ss->ssl3.hs.rtTimerCb = cb;

    return SECSuccess;
}





SECStatus
dtls_RestartTimer(sslSocket *ss, PRBool backoff, DTLSTimerCb cb)
{
    if (backoff) {
        ss->ssl3.hs.rtTimeoutMs *= 2;
        if (ss->ssl3.hs.rtTimeoutMs > MAX_DTLS_TIMEOUT_MS)
            ss->ssl3.hs.rtTimeoutMs = MAX_DTLS_TIMEOUT_MS;
    }

    return dtls_StartTimer(ss, cb);
}







void
dtls_CancelTimer(sslSocket *ss)
{
    PORT_Assert(ss->opt.noLocks || ssl_HaveRecvBufLock(ss));

    ss->ssl3.hs.rtTimerCb = NULL;
}





void
dtls_CheckTimer(sslSocket *ss)
{
    if (!ss->ssl3.hs.rtTimerCb)
        return;

    if ((PR_IntervalNow() - ss->ssl3.hs.rtTimerStarted) >
        PR_MillisecondsToInterval(ss->ssl3.hs.rtTimeoutMs)) {
        
        DTLSTimerCb cb = ss->ssl3.hs.rtTimerCb;

        
        dtls_CancelTimer(ss);

        
        cb(ss);
    }
}






void
dtls_FinishedTimerCb(sslSocket *ss)
{
    ssl3_DestroyCipherSpec(ss->ssl3.pwSpec, PR_FALSE);
}









void
dtls_RehandshakeCleanup(sslSocket *ss)
{
    dtls_CancelTimer(ss);
    ssl3_DestroyCipherSpec(ss->ssl3.pwSpec, PR_FALSE);
    ss->ssl3.hs.sendMessageSeq = 0;
    ss->ssl3.hs.recvMessageSeq = 0;
}












void
dtls_SetMTU(sslSocket *ss, PRUint16 advertised)
{
    int i;

    if (advertised == 0) {
        ss->ssl3.mtu = COMMON_MTU_VALUES[0];
        SSL_TRC(30, ("Resetting MTU to %d", ss->ssl3.mtu));
        return;
    }

    for (i = 0; i < PR_ARRAY_SIZE(COMMON_MTU_VALUES); i++) {
        if (COMMON_MTU_VALUES[i] <= advertised) {
            ss->ssl3.mtu = COMMON_MTU_VALUES[i];
            SSL_TRC(30, ("Resetting MTU to %d", ss->ssl3.mtu));
            return;
        }
    }

    
    ss->ssl3.mtu = COMMON_MTU_VALUES[PR_ARRAY_SIZE(COMMON_MTU_VALUES)-1];
    SSL_TRC(30, ("Resetting MTU to %d", ss->ssl3.mtu));
}





SECStatus
dtls_HandleHelloVerifyRequest(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    int                 errCode = SSL_ERROR_RX_MALFORMED_HELLO_VERIFY_REQUEST;
    SECStatus           rv;
    PRInt32             temp;
    SECItem             cookie = {siBuffer, NULL, 0};
    SSL3AlertDescription desc   = illegal_parameter;

    SSL_TRC(3, ("%d: SSL3[%d]: handle hello_verify_request handshake",
        SSL_GETPID(), ss->fd));
    PORT_Assert(ss->opt.noLocks || ssl_HaveRecvBufLock(ss));
    PORT_Assert(ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss));

    if (ss->ssl3.hs.ws != wait_server_hello) {
        errCode = SSL_ERROR_RX_UNEXPECTED_HELLO_VERIFY_REQUEST;
        desc    = unexpected_message;
        goto alert_loser;
    }

    
    temp = ssl3_ConsumeHandshakeNumber(ss, 2, &b, &length);
    if (temp < 0) {
        goto loser;     
    }

    if (temp != SSL_LIBRARY_VERSION_DTLS_1_0_WIRE &&
        temp != SSL_LIBRARY_VERSION_DTLS_1_2_WIRE) {
        goto alert_loser;
    }

    
    rv = ssl3_ConsumeHandshakeVariable(ss, &cookie, 1, &b, &length);
    if (rv != SECSuccess) {
        goto loser;     
    }
    if (cookie.len > DTLS_COOKIE_BYTES) {
        desc = decode_error;
        goto alert_loser;       
    }

    PORT_Memcpy(ss->ssl3.hs.cookie, cookie.data, cookie.len);
    ss->ssl3.hs.cookieLen = cookie.len;


    ssl_GetXmitBufLock(ss);             

    
    rv = ssl3_SendClientHello(ss, PR_TRUE);

    ssl_ReleaseXmitBufLock(ss);         

    if (rv == SECSuccess)
        return rv;

alert_loser:
    (void)SSL3_SendAlert(ss, alert_fatal, desc);

loser:
    errCode = ssl_MapLowLevelError(errCode);
    return SECFailure;
}







void
dtls_InitRecvdRecords(DTLSRecvdRecords *records)
{
    PORT_Memset(records->data, 0, sizeof(records->data));
    records->left = 0;
    records->right = DTLS_RECVD_RECORDS_WINDOW - 1;
}









int
dtls_RecordGetRecvd(DTLSRecvdRecords *records, PRUint64 seq)
{
    PRUint64 offset;

    
    if (seq < records->left) {
        return -1;
    }

    


    if (seq > records->right)
        return 0;

    offset = seq % DTLS_RECVD_RECORDS_WINDOW;

    return !!(records->data[offset / 8] & (1 << (offset % 8)));
}





void
dtls_RecordSetRecvd(DTLSRecvdRecords *records, PRUint64 seq)
{
    PRUint64 offset;

    if (seq < records->left)
        return;

    if (seq > records->right) {
        PRUint64 new_left;
        PRUint64 new_right;
        PRUint64 right;

        










        new_right = seq | 0x07;
        new_left = (new_right - DTLS_RECVD_RECORDS_WINDOW) + 1;

        for (right = records->right + 8; right <= new_right; right += 8) {
            offset = right % DTLS_RECVD_RECORDS_WINDOW;
            records->data[offset / 8] = 0;
        }

        records->right = new_right;
        records->left = new_left;
    }

    offset = seq % DTLS_RECVD_RECORDS_WINDOW;

    records->data[offset / 8] |= (1 << (offset % 8));
}

SECStatus
DTLS_GetHandshakeTimeout(PRFileDesc *socket, PRIntervalTime *timeout)
{
    sslSocket * ss = NULL;
    PRIntervalTime elapsed;
    PRIntervalTime desired;

    ss = ssl_FindSocket(socket);

    if (!ss)
        return SECFailure;

    if (!IS_DTLS(ss))
        return SECFailure;

    if (!ss->ssl3.hs.rtTimerCb)
        return SECFailure;

    elapsed = PR_IntervalNow() - ss->ssl3.hs.rtTimerStarted;
    desired = PR_MillisecondsToInterval(ss->ssl3.hs.rtTimeoutMs);
    if (elapsed > desired) {
        
        *timeout = PR_INTERVAL_NO_WAIT;
    } else {
        *timeout = desired - elapsed;
    }

    return SECSuccess;
}
