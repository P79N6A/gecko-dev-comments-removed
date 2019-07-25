






































#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"


static SECStatus ssl2_HandleV3HandshakeRecord(sslSocket *ss);







































int 
ssl2_GatherData(sslSocket *ss, sslGather *gs, int flags)
{
    unsigned char *  bp;
    unsigned char *  pBuf;
    int              nb, err, rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );

    if (gs->state == GS_INIT) {
	
	gs->state         = GS_HEADER;
	gs->remainder     = 3;
	gs->count         = 3;
	gs->offset        = 0;
	gs->recordLen     = 0;
	gs->recordPadding = 0;
	gs->hdr[2]        = 0;

	gs->writeOffset   = 0;
	gs->readOffset    = 0;
    }
    if (gs->encrypted) {
	PORT_Assert(ss->sec.hash != 0);
    }

    pBuf = gs->buf.buf;
    for (;;) {
	SSL_TRC(30, ("%d: SSL[%d]: gather state %d (need %d more)",
		     SSL_GETPID(), ss->fd, gs->state, gs->remainder));
	bp = ((gs->state != GS_HEADER) ? pBuf : gs->hdr) + gs->offset;
	nb = ssl_DefRecv(ss, bp, gs->remainder, flags);
	if (nb > 0) {
	    PRINT_BUF(60, (ss, "raw gather data:", bp, nb));
	}
	if (nb == 0) {
	    
	    SSL_TRC(30, ("%d: SSL[%d]: EOF", SSL_GETPID(), ss->fd));
	    rv = 0;
	    break;
	}
	if (nb < 0) {
	    SSL_DBG(("%d: SSL[%d]: recv error %d", SSL_GETPID(), ss->fd,
		     PR_GetError()));
	    rv = SECFailure;
	    break;
	}

	gs->offset    += nb;
	gs->remainder -= nb;

	if (gs->remainder > 0) {
	    continue;
	}

	
	switch (gs->state) {
	case GS_HEADER: 
	    if ((ss->opt.enableSSL3 || ss->opt.enableTLS) && !ss->firstHsDone) {

		PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

		



		if (gs->hdr[0] == content_handshake) {
		    if ((ss->nextHandshake == ssl2_HandleClientHelloMessage) ||
			(ss->nextHandshake == ssl2_HandleServerHelloMessage)) {
			rv = ssl2_HandleV3HandshakeRecord(ss);
			if (rv == SECFailure) {
			    return SECFailure;
			}
		    }
		    
















		    return SECWouldBlock;
		} else if (gs->hdr[0] == content_alert) {
		    if (ss->nextHandshake == ssl2_HandleServerHelloMessage) {
			



			PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
			return SECFailure;
		    }
		}
	    }	

	    
	    if (gs->hdr[0] & 0x80) {
		
		gs->count = ((gs->hdr[0] & 0x7f) << 8) | gs->hdr[1];
		gs->recordPadding = 0;
	    } else {
		
		gs->count = ((gs->hdr[0] & 0x3f) << 8) | gs->hdr[1];
	    
		gs->recordPadding = gs->hdr[2];
	    }
	    if (!gs->count) {
		PORT_SetError(SSL_ERROR_RX_RECORD_TOO_LONG);
		goto cleanup;
	    }

	    if (gs->count > gs->buf.space) {
		err = sslBuffer_Grow(&gs->buf, gs->count);
		if (err) {
		    return err;
		}
		pBuf = gs->buf.buf;
	    }


	    if (gs->hdr[0] & 0x80) {
	    	


		pBuf[0]        = gs->hdr[2];
		gs->offset    = 1;
		gs->remainder = gs->count - 1;
	    } else {
		gs->offset    = 0;
		gs->remainder = gs->count;
	    }

	    if (gs->encrypted) {
		gs->state     = GS_MAC;
		gs->recordLen = gs->count - gs->recordPadding
		                          - ss->sec.hash->length;
	    } else {
		gs->state     = GS_DATA;
		gs->recordLen = gs->count;
	    }

	    break;


	case GS_MAC:
	    




	    PORT_Assert(gs->encrypted);

	  {
	    unsigned int     macLen;
	    int              nout;
	    unsigned char    mac[SSL_MAX_MAC_BYTES];

	    ssl_GetSpecReadLock(ss); 

	    




	    if (gs->count & (ss->sec.blockSize - 1)) {
		
		SSL_DBG(("%d: SSL[%d]: sender, count=%d blockSize=%d",
			 SSL_GETPID(), ss->fd, gs->count,
			 ss->sec.blockSize));
		PORT_SetError(SSL_ERROR_BAD_BLOCK_PADDING);
		rv = SECFailure;
		goto spec_locked_done;
	    }
	    PORT_Assert(gs->count == gs->offset);

	    if (gs->offset == 0) {
		rv = 0;			
		goto spec_locked_done;
	    }

	    


	    rv = (*ss->sec.dec)(ss->sec.readcx, pBuf, &nout, gs->offset,
			     pBuf, gs->offset);
	    if (rv != SECSuccess) {
		goto spec_locked_done;
	    }


	    



	    macLen = ss->sec.hash->length;
	    if (gs->offset >= macLen) {
		PRUint32           sequenceNumber = ss->sec.rcvSequence++;
		unsigned char    seq[4];

		seq[0] = (unsigned char) (sequenceNumber >> 24);
		seq[1] = (unsigned char) (sequenceNumber >> 16);
		seq[2] = (unsigned char) (sequenceNumber >> 8);
		seq[3] = (unsigned char) (sequenceNumber);

		(*ss->sec.hash->begin)(ss->sec.hashcx);
		(*ss->sec.hash->update)(ss->sec.hashcx, ss->sec.rcvSecret.data,
				        ss->sec.rcvSecret.len);
		(*ss->sec.hash->update)(ss->sec.hashcx, pBuf + macLen, 
				        gs->offset - macLen);
		(*ss->sec.hash->update)(ss->sec.hashcx, seq, 4);
		(*ss->sec.hash->end)(ss->sec.hashcx, mac, &macLen, macLen);

		PORT_Assert(macLen == ss->sec.hash->length);

		ssl_ReleaseSpecReadLock(ss);  

		if (NSS_SecureMemcmp(mac, pBuf, macLen) != 0) {
		    
		    SSL_DBG(("%d: SSL[%d]: mac check failed, seq=%d",
			     SSL_GETPID(), ss->fd, ss->sec.rcvSequence));
		    PRINT_BUF(1, (ss, "computed mac:", mac, macLen));
		    PRINT_BUF(1, (ss, "received mac:", pBuf, macLen));
		    PORT_SetError(SSL_ERROR_BAD_MAC_READ);
		    rv = SECFailure;
		    goto cleanup;
		}
	    } else {
		ssl_ReleaseSpecReadLock(ss);  
	    }

	    if (gs->recordPadding + macLen <= gs->offset) {
		gs->recordOffset  = macLen;
		gs->readOffset    = macLen;
		gs->writeOffset   = gs->offset - gs->recordPadding;
		rv = 1;
	    } else {
		PORT_SetError(SSL_ERROR_BAD_BLOCK_PADDING);
cleanup:
		
		gs->recordOffset  = 0;
		gs->readOffset    = 0;
	    	gs->writeOffset   = 0;
		rv = SECFailure;
	    }

	    gs->recordLen     = gs->writeOffset - gs->readOffset;
	    gs->recordPadding = 0;	
	    gs->state = GS_INIT;


	    if (rv > 0) {
		PRINT_BUF(50, (ss, "recv clear record:", 
		               pBuf + gs->recordOffset, gs->recordLen));
	    }
	    return rv;

spec_locked_done:
	    ssl_ReleaseSpecReadLock(ss);
	    return rv;
	  }

	case GS_DATA:
	    

	    gs->recordOffset  = 0;
	    gs->readOffset    = 0;
	    gs->writeOffset   = gs->offset;
	    PORT_Assert(gs->recordLen == gs->writeOffset - gs->readOffset);
	    gs->recordLen     = gs->offset;
	    gs->recordPadding = 0;
	    gs->state         = GS_INIT;

	    ++ss->sec.rcvSequence;

	    PRINT_BUF(50, (ss, "recv clear record:", 
	                   pBuf + gs->recordOffset, gs->recordLen));
	    return 1;

	}	
    }		
    return rv;
}

















int 
ssl2_GatherRecord(sslSocket *ss, int flags)
{
    return ssl2_GatherData(ss, &ss->gs, flags);
}










int 
ssl2_StartGatherBytes(sslSocket *ss, sslGather *gs, unsigned int count)
{
    int rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    gs->state     = GS_DATA;
    gs->remainder = count;
    gs->count     = count;
    gs->offset    = 0;
    if (count > gs->buf.space) {
	rv = sslBuffer_Grow(&gs->buf, count);
	if (rv) {
	    return rv;
	}
    }
    return ssl2_GatherData(ss, gs, 0);
}


SECStatus
ssl_InitGather(sslGather *gs)
{
    SECStatus status;

    gs->state = GS_INIT;
    gs->writeOffset = 0;
    gs->readOffset  = 0;
    status = sslBuffer_Grow(&gs->buf, 4096);
    return status;
}


void 
ssl_DestroyGather(sslGather *gs)
{
    if (gs) {	
	PORT_ZFree(gs->buf.buf, gs->buf.space);
	PORT_Free(gs->inbuf.buf);
    }
}


static SECStatus
ssl2_HandleV3HandshakeRecord(sslSocket *ss)
{
    SECStatus           rv;
    SSL3ProtocolVersion version = (ss->gs.hdr[1] << 8) | ss->gs.hdr[2];

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_Have1stHandshakeLock(ss) );

    
    ss->gs.remainder         = 2;
    ss->gs.count             = 0;

    


    ss->nextHandshake     = 0;
    ss->securityHandshake = 0;

    



    rv = ssl3_NegotiateVersion(ss, version);
    if (rv != SECSuccess) {
	return rv;
    }

    ss->sec.send         = ssl3_SendApplicationData;

    return SECSuccess;
}
