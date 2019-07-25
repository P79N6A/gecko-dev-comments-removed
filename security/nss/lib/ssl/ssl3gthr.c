







































#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"
#include "ssl3prot.h"





















static int
ssl3_GatherData(sslSocket *ss, sslGather *gs, int flags)
{
    unsigned char *bp;
    unsigned char *lbp;
    int            nb;
    int            err;
    int            rv		= 1;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    if (gs->state == GS_INIT) {
	gs->state       = GS_HEADER;
	gs->remainder   = 5;
	gs->offset      = 0;
	gs->writeOffset = 0;
	gs->readOffset  = 0;
	gs->inbuf.len   = 0;
    }
    
    lbp = gs->inbuf.buf;
    for(;;) {
	SSL_TRC(30, ("%d: SSL3[%d]: gather state %d (need %d more)",
		SSL_GETPID(), ss->fd, gs->state, gs->remainder));
	bp = ((gs->state != GS_HEADER) ? lbp : gs->hdr) + gs->offset;
	nb = ssl_DefRecv(ss, bp, gs->remainder, flags);

	if (nb > 0) {
	    PRINT_BUF(60, (ss, "raw gather data:", bp, nb));
	} else if (nb == 0) {
	    
	    SSL_TRC(30, ("%d: SSL3[%d]: EOF", SSL_GETPID(), ss->fd));
	    rv = 0;
	    break;
	} else  {
	    SSL_DBG(("%d: SSL3[%d]: recv error %d", SSL_GETPID(), ss->fd,
		     PR_GetError()));
	    rv = SECFailure;
	    break;
	}

	PORT_Assert( nb <= gs->remainder );
	if (nb > gs->remainder) {
	    
	    gs->state = GS_INIT;         
	    rv = SECFailure;
	    break;
	}

	gs->offset    += nb;
	gs->remainder -= nb;
	if (gs->state == GS_DATA)
	    gs->inbuf.len += nb;

	
	if (gs->remainder > 0) {
	    continue;
	}

	
	switch (gs->state) {
	case GS_HEADER:
	    




	    gs->remainder = (gs->hdr[3] << 8) | gs->hdr[4];

	    


	    if(gs->remainder > (MAX_FRAGMENT_LENGTH + 2048 + 5)) {
		SSL3_SendAlert(ss, alert_fatal, unexpected_message);
		gs->state = GS_INIT;
		PORT_SetError(SSL_ERROR_RX_RECORD_TOO_LONG);
		return SECFailure;
	    }

	    gs->state     = GS_DATA;
	    gs->offset    = 0;
	    gs->inbuf.len = 0;

	    if (gs->remainder > gs->inbuf.space) {
		err = sslBuffer_Grow(&gs->inbuf, gs->remainder);
		if (err) {	
		    return err;
		}
		lbp = gs->inbuf.buf;
	    }
	    break;	


	case GS_DATA:
	    


	    gs->state = GS_INIT;
	    return 1;
	}
    }

    return rv;
}

















int
ssl3_GatherCompleteHandshake(sslSocket *ss, int flags)
{
    SSL3Ciphertext cText;
    int            rv;
    PRBool         canFalseStart = PR_FALSE;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    do {
        



        ssl_GetSSL3HandshakeLock(ss);
        rv = ss->ssl3.hs.restartTarget == NULL ? SECSuccess : SECFailure;
        ssl_ReleaseSSL3HandshakeLock(ss);
        if (rv != SECSuccess) {
            PORT_SetError(PR_WOULD_BLOCK_ERROR);
            return (int) SECFailure;
        }

        




        if (ss->ssl3.hs.msgState.buf != NULL) {
            if (ss->ssl3.hs.msgState.len == 0) {
                ss->ssl3.hs.msgState.buf = NULL;
            }
        }

        if (ss->ssl3.hs.msgState.buf != NULL) {
            




	    rv = ssl3_HandleRecord(ss, NULL, &ss->gs.buf);
        } else {
	    
	    rv = ssl3_GatherData(ss, &ss->gs, flags);
	    if (rv <= 0) {
	        return rv;
	    }

	    




	    cText.type    = (SSL3ContentType)ss->gs.hdr[0];
	    cText.version = (ss->gs.hdr[1] << 8) | ss->gs.hdr[2];
	    cText.buf     = &ss->gs.inbuf;
	    rv = ssl3_HandleRecord(ss, &cText, &ss->gs.buf);
        }
	if (rv < 0) {
	    return ss->recvdCloseNotify ? 0 : rv;
	}

	


	if (ss->opt.enableFalseStart) {
	    ssl_GetSSL3HandshakeLock(ss);
	    canFalseStart = (ss->ssl3.hs.ws == wait_change_cipher ||
			     ss->ssl3.hs.ws == wait_new_session_ticket) &&
		            ssl3_CanFalseStart(ss);
	    ssl_ReleaseSSL3HandshakeLock(ss);
	}
    } while (ss->ssl3.hs.ws != idle_handshake &&
             !canFalseStart &&
             ss->gs.buf.len == 0);

    ss->gs.readOffset = 0;
    ss->gs.writeOffset = ss->gs.buf.len;
    return 1;
}












int
ssl3_GatherAppDataRecord(sslSocket *ss, int flags)
{
    int            rv;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    do {
	rv = ssl3_GatherCompleteHandshake(ss, flags);
    } while (rv > 0 && ss->gs.buf.len == 0);

    return rv;
}
