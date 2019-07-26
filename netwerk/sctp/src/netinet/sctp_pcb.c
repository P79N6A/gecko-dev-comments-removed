































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_pcb.c 246687 2013-02-11 21:02:49Z tuexen $");
#endif

#include <netinet/sctp_os.h>
#ifdef __FreeBSD__
#include <sys/proc.h>
#endif
#include <netinet/sctp_var.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctputil.h>
#include <netinet/sctp.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctp_bsd_addr.h>
#if defined(__FreeBSD__) && __FreeBSD_version >= 803000
#include <netinet/sctp_dtrace_define.h>
#endif
#if !defined(__Userspace_os_Windows)
#include <netinet/udp.h>
#endif
#ifdef INET6
#if defined(__Userspace__)
#include "user_ip6_var.h"
#else
#include <netinet6/ip6_var.h>
#endif
#endif
#if defined(__FreeBSD__)
#include <sys/sched.h>
#include <sys/smp.h>
#include <sys/unistd.h>
#endif
#if defined(__Userspace__)
#if !defined(__Userspace_os_Windows)
#if defined(ANDROID)
#include <unistd.h>
#include <ifaddrs-android-ext.h>
#else
#include <sys/unistd.h>
#endif
#endif
#include <user_socketvar.h>
#endif

#if defined(__APPLE__)
#define APPLE_FILE_NO 4
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
VNET_DEFINE(struct sctp_base_info, system_base_info);
#else
struct sctp_base_info system_base_info;
#endif

#if defined(__Userspace__)
struct ifaddrs *g_interfaces;
#endif


#ifdef INET6
int
SCTP6_ARE_ADDR_EQUAL(struct sockaddr_in6 *a, struct sockaddr_in6 *b)
{
#ifdef SCTP_EMBEDDED_V6_SCOPE
#if defined(__APPLE__)
	struct in6_addr tmp_a, tmp_b;

	tmp_a = a->sin6_addr;
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
	if (in6_embedscope(&tmp_a, a, NULL, NULL) != 0) {
#else
	if (in6_embedscope(&tmp_a, a, NULL, NULL, NULL) != 0) {
#endif
		return (0);
	}
	tmp_b = b->sin6_addr;
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
	if (in6_embedscope(&tmp_b, b, NULL, NULL) != 0) {
#else
	if (in6_embedscope(&tmp_b, b, NULL, NULL, NULL) != 0) {
#endif
		return (0);
	}
	return (IN6_ARE_ADDR_EQUAL(&tmp_a, &tmp_b));
#elif defined(SCTP_KAME)
	struct sockaddr_in6 tmp_a, tmp_b;

	memcpy(&tmp_a, a, sizeof(struct sockaddr_in6));
	if (sa6_embedscope(&tmp_a, MODULE_GLOBAL(ip6_use_defzone)) != 0) {
		return (0);
	}
	memcpy(&tmp_b, b, sizeof(struct sockaddr_in6));
	if (sa6_embedscope(&tmp_b, MODULE_GLOBAL(ip6_use_defzone)) != 0) {
		return (0);
	}
	return (IN6_ARE_ADDR_EQUAL(&tmp_a.sin6_addr, &tmp_b.sin6_addr));
#else
	struct in6_addr tmp_a, tmp_b;

	tmp_a = a->sin6_addr;
	if (in6_embedscope(&tmp_a, a) != 0) {
		return (0);
	}
	tmp_b = b->sin6_addr;
	if (in6_embedscope(&tmp_b, b) != 0) {
		return (0);
	}
	return (IN6_ARE_ADDR_EQUAL(&tmp_a, &tmp_b));
#endif
#else
	return (IN6_ARE_ADDR_EQUAL(&(a->sin6_addr), &(b->sin6_addr)));
#endif 
}
#endif

void
sctp_fill_pcbinfo(struct sctp_pcbinfo *spcb)
{
	



	SCTP_INP_INFO_RLOCK();
	spcb->ep_count = SCTP_BASE_INFO(ipi_count_ep);
	spcb->asoc_count = SCTP_BASE_INFO(ipi_count_asoc);
	spcb->laddr_count = SCTP_BASE_INFO(ipi_count_laddr);
	spcb->raddr_count = SCTP_BASE_INFO(ipi_count_raddr);
	spcb->chk_count = SCTP_BASE_INFO(ipi_count_chunk);
	spcb->readq_count = SCTP_BASE_INFO(ipi_count_readq);
	spcb->stream_oque = SCTP_BASE_INFO(ipi_count_strmoq);
	spcb->free_chunks = SCTP_BASE_INFO(ipi_free_chunks);
	SCTP_INP_INFO_RUNLOCK();
}














































struct sctp_vrf *
sctp_allocate_vrf(int vrf_id)
{
	struct sctp_vrf *vrf = NULL;
	struct sctp_vrflist *bucket;

	
	vrf = sctp_find_vrf(vrf_id);
	if (vrf) {
		
		return (vrf);
	}
	SCTP_MALLOC(vrf, struct sctp_vrf *, sizeof(struct sctp_vrf),
		    SCTP_M_VRF);
 	if (vrf == NULL) {
 		
#ifdef INVARIANTS
		panic("No memory for VRF:%d", vrf_id);
#endif
		return (NULL);
	}
	
	memset(vrf, 0, sizeof(struct sctp_vrf));
	vrf->vrf_id = vrf_id;
	LIST_INIT(&vrf->ifnlist);
	vrf->total_ifa_count = 0;
	vrf->refcount = 0;
	
	SCTP_INIT_VRF_TABLEID(vrf);
	
	vrf->vrf_addr_hash = SCTP_HASH_INIT(SCTP_VRF_ADDR_HASH_SIZE,
					    &vrf->vrf_addr_hashmark);
	if (vrf->vrf_addr_hash == NULL) {
 		
#ifdef INVARIANTS
		panic("No memory for VRF:%d", vrf_id);
#endif
		SCTP_FREE(vrf, SCTP_M_VRF);
		return (NULL);
	}

	
	bucket = &SCTP_BASE_INFO(sctp_vrfhash)[(vrf_id & SCTP_BASE_INFO(hashvrfmark))];
	LIST_INSERT_HEAD(bucket, vrf, next_vrf);
	atomic_add_int(&SCTP_BASE_INFO(ipi_count_vrfs), 1);
	return (vrf);
}


struct sctp_ifn *
sctp_find_ifn(void *ifn, uint32_t ifn_index)
{
	struct sctp_ifn *sctp_ifnp;
	struct sctp_ifnlist *hash_ifn_head;

	


	hash_ifn_head = &SCTP_BASE_INFO(vrf_ifn_hash)[(ifn_index & SCTP_BASE_INFO(vrf_ifn_hashmark))];
	LIST_FOREACH(sctp_ifnp, hash_ifn_head, next_bucket) {
		if (sctp_ifnp->ifn_index == ifn_index) {
			return (sctp_ifnp);
		}
		if (sctp_ifnp->ifn_p && ifn && (sctp_ifnp->ifn_p == ifn)) {
			return (sctp_ifnp);
		}
	}
	return (NULL);
}


struct sctp_vrf *
sctp_find_vrf(uint32_t vrf_id)
{
	struct sctp_vrflist *bucket;
	struct sctp_vrf *liste;

	bucket = &SCTP_BASE_INFO(sctp_vrfhash)[(vrf_id & SCTP_BASE_INFO(hashvrfmark))];
	LIST_FOREACH(liste, bucket, next_vrf) {
		if (vrf_id == liste->vrf_id) {
			return (liste);
		}
	}
	return (NULL);
}


void
sctp_free_vrf(struct sctp_vrf *vrf)
{
	if (SCTP_DECREMENT_AND_CHECK_REFCOUNT(&vrf->refcount)) {
                if (vrf->vrf_addr_hash) {
                    SCTP_HASH_FREE(vrf->vrf_addr_hash, vrf->vrf_addr_hashmark);
                    vrf->vrf_addr_hash = NULL;
                }
		
		LIST_REMOVE(vrf, next_vrf);
		SCTP_FREE(vrf, SCTP_M_VRF);
		atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_vrfs), 1);
	}
}


void
sctp_free_ifn(struct sctp_ifn *sctp_ifnp)
{
	if (SCTP_DECREMENT_AND_CHECK_REFCOUNT(&sctp_ifnp->refcount)) {
		
		if (sctp_ifnp->vrf) {
			sctp_free_vrf(sctp_ifnp->vrf);
		}
		SCTP_FREE(sctp_ifnp, SCTP_M_IFN);
		atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_ifns), 1);
	}
}


void
sctp_update_ifn_mtu(uint32_t ifn_index, uint32_t mtu)
{
	struct sctp_ifn *sctp_ifnp;

	sctp_ifnp = sctp_find_ifn((void *)NULL, ifn_index);
	if (sctp_ifnp != NULL) {
		sctp_ifnp->ifn_mtu = mtu;
	}
}


void
sctp_free_ifa(struct sctp_ifa *sctp_ifap)
{
	if (SCTP_DECREMENT_AND_CHECK_REFCOUNT(&sctp_ifap->refcount)) {
		
		if (sctp_ifap->ifn_p) {
			sctp_free_ifn(sctp_ifap->ifn_p);
		}
		SCTP_FREE(sctp_ifap, SCTP_M_IFA);
		atomic_subtract_int(&SCTP_BASE_INFO(ipi_count_ifas), 1);
	}
}


static void
sctp_delete_ifn(struct sctp_ifn *sctp_ifnp, int hold_addr_lock)
{
	struct sctp_ifn *found;

	found = sctp_find_ifn(sctp_ifnp->ifn_p, sctp_ifnp->ifn_index);
	if (found == NULL) {
		
		return;
	}
	if (hold_addr_lock == 0)
		SCTP_IPI_ADDR_WLOCK();
	LIST_REMOVE(sctp_ifnp, next_bucket);
	LIST_REMOVE(sctp_ifnp, next_ifn);
	SCTP_DEREGISTER_INTERFACE(sctp_ifnp->ifn_index,
				  sctp_ifnp->registered_af);
	if (hold_addr_lock == 0)
		SCTP_IPI_ADDR_WUNLOCK();
	
	sctp_free_ifn(sctp_ifnp);
}


void
sctp_mark_ifa_addr_down(uint32_t vrf_id, struct sockaddr *addr,
			const char *if_name, uint32_t ifn_index)
{
	struct sctp_vrf *vrf;
	struct sctp_ifa *sctp_ifap;

	SCTP_IPI_ADDR_RLOCK();
	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "Can't find vrf_id 0x%x\n", vrf_id);
		goto out;

	}
	sctp_ifap = sctp_find_ifa_by_addr(addr, vrf->vrf_id, SCTP_ADDR_LOCKED);
	if (sctp_ifap == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "Can't find sctp_ifap for address\n");
		goto out;
	}
	if (sctp_ifap->ifn_p == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "IFA has no IFN - can't mark unuseable\n");
		goto out;
	}
	if (if_name) {
		if (strncmp(if_name, sctp_ifap->ifn_p->ifn_name, SCTP_IFNAMSIZ) != 0) {
			SCTPDBG(SCTP_DEBUG_PCB4, "IFN %s of IFA not the same as %s\n",
				sctp_ifap->ifn_p->ifn_name, if_name);
			goto out;
		}
	} else {
		if (sctp_ifap->ifn_p->ifn_index != ifn_index) {
			SCTPDBG(SCTP_DEBUG_PCB4, "IFA owned by ifn_index:%d down command for ifn_index:%d - ignored\n",
				sctp_ifap->ifn_p->ifn_index, ifn_index);
			goto out;
		}
	}

	sctp_ifap->localifa_flags &= (~SCTP_ADDR_VALID);
	sctp_ifap->localifa_flags |= SCTP_ADDR_IFA_UNUSEABLE;
 out:
	SCTP_IPI_ADDR_RUNLOCK();
}


void
sctp_mark_ifa_addr_up(uint32_t vrf_id, struct sockaddr *addr,
		      const char *if_name, uint32_t ifn_index)
{
	struct sctp_vrf *vrf;
	struct sctp_ifa *sctp_ifap;

	SCTP_IPI_ADDR_RLOCK();
	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "Can't find vrf_id 0x%x\n", vrf_id);
		goto out;

	}
	sctp_ifap = sctp_find_ifa_by_addr(addr, vrf->vrf_id, SCTP_ADDR_LOCKED);
	if (sctp_ifap == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "Can't find sctp_ifap for address\n");
		goto out;
	}
	if (sctp_ifap->ifn_p == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "IFA has no IFN - can't mark unuseable\n");
		goto out;
	}
	if (if_name) {
		if (strncmp(if_name, sctp_ifap->ifn_p->ifn_name, SCTP_IFNAMSIZ) != 0) {
			SCTPDBG(SCTP_DEBUG_PCB4, "IFN %s of IFA not the same as %s\n",
				sctp_ifap->ifn_p->ifn_name, if_name);
			goto out;
		}
	} else {
		if (sctp_ifap->ifn_p->ifn_index != ifn_index) {
			SCTPDBG(SCTP_DEBUG_PCB4, "IFA owned by ifn_index:%d down command for ifn_index:%d - ignored\n",
				sctp_ifap->ifn_p->ifn_index, ifn_index);
			goto out;
		}
	}

	sctp_ifap->localifa_flags &= (~SCTP_ADDR_IFA_UNUSEABLE);
	sctp_ifap->localifa_flags |= SCTP_ADDR_VALID;
 out:
	SCTP_IPI_ADDR_RUNLOCK();
}







static void
sctp_add_ifa_to_ifn(struct sctp_ifn *sctp_ifnp, struct sctp_ifa *sctp_ifap)
{
	int ifa_af;

	LIST_INSERT_HEAD(&sctp_ifnp->ifalist, sctp_ifap, next_ifa);
	sctp_ifap->ifn_p = sctp_ifnp;
	atomic_add_int(&sctp_ifap->ifn_p->refcount, 1);
	
	sctp_ifnp->ifa_count++;
	ifa_af = sctp_ifap->address.sa.sa_family;
	switch (ifa_af) {
#ifdef INET
	case AF_INET:
		sctp_ifnp->num_v4++;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		sctp_ifnp->num_v6++;
		break;
#endif
	default:
		break;
	}
	if (sctp_ifnp->ifa_count == 1) {
		
		SCTP_REGISTER_INTERFACE(sctp_ifnp->ifn_index, ifa_af);
		sctp_ifnp->registered_af = ifa_af;
	}
}








static void
sctp_remove_ifa_from_ifn(struct sctp_ifa *sctp_ifap)
{
	LIST_REMOVE(sctp_ifap, next_ifa);
	if (sctp_ifap->ifn_p) {
		
		sctp_ifap->ifn_p->ifa_count--;
		switch (sctp_ifap->address.sa.sa_family) {
#ifdef INET
		case AF_INET:
			sctp_ifap->ifn_p->num_v4--;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			sctp_ifap->ifn_p->num_v6--;
			break;
#endif
		default:
			break;
		}

		if (LIST_EMPTY(&sctp_ifap->ifn_p->ifalist)) {
			
			sctp_delete_ifn(sctp_ifap->ifn_p, SCTP_ADDR_LOCKED);
		} else {
			
			if ((sctp_ifap->ifn_p->num_v6 == 0) &&
			    (sctp_ifap->ifn_p->registered_af == AF_INET6)) {
				SCTP_DEREGISTER_INTERFACE(sctp_ifap->ifn_p->ifn_index, AF_INET6);
				SCTP_REGISTER_INTERFACE(sctp_ifap->ifn_p->ifn_index, AF_INET);
				sctp_ifap->ifn_p->registered_af = AF_INET;
			} else if ((sctp_ifap->ifn_p->num_v4 == 0) &&
				   (sctp_ifap->ifn_p->registered_af == AF_INET)) {
				SCTP_DEREGISTER_INTERFACE(sctp_ifap->ifn_p->ifn_index, AF_INET);
				SCTP_REGISTER_INTERFACE(sctp_ifap->ifn_p->ifn_index, AF_INET6);
				sctp_ifap->ifn_p->registered_af = AF_INET6;
			}
			
			sctp_free_ifn(sctp_ifap->ifn_p);
		}
		sctp_ifap->ifn_p = NULL;
	}
}


struct sctp_ifa *
sctp_add_addr_to_vrf(uint32_t vrf_id, void *ifn, uint32_t ifn_index,
		     uint32_t ifn_type, const char *if_name, void *ifa,
		     struct sockaddr *addr, uint32_t ifa_flags,
		     int dynamic_add)
{
	struct sctp_vrf *vrf;
	struct sctp_ifn *sctp_ifnp = NULL;
	struct sctp_ifa *sctp_ifap = NULL;
	struct sctp_ifalist *hash_addr_head;
	struct sctp_ifnlist *hash_ifn_head;
	uint32_t hash_of_addr;
	int new_ifn_af = 0;

#ifdef SCTP_DEBUG
	SCTPDBG(SCTP_DEBUG_PCB4, "vrf_id 0x%x: adding address: ", vrf_id);
	SCTPDBG_ADDR(SCTP_DEBUG_PCB4, addr);
#endif
	SCTP_IPI_ADDR_WLOCK();
	sctp_ifnp = sctp_find_ifn(ifn, ifn_index);
	if (sctp_ifnp) {
		vrf = sctp_ifnp->vrf;
	} else {
		vrf = sctp_find_vrf(vrf_id);
		if (vrf == NULL) {
			vrf = sctp_allocate_vrf(vrf_id);
			if (vrf == NULL) {
				SCTP_IPI_ADDR_WUNLOCK();
				return (NULL);
			}
		}
	}
	if (sctp_ifnp == NULL) {
		


		SCTP_IPI_ADDR_WUNLOCK();
		SCTP_MALLOC(sctp_ifnp, struct sctp_ifn *,
			    sizeof(struct sctp_ifn), SCTP_M_IFN);
		if (sctp_ifnp == NULL) {
#ifdef INVARIANTS
			panic("No memory for IFN");
#endif
			return (NULL);
		}
		memset(sctp_ifnp, 0, sizeof(struct sctp_ifn));
		sctp_ifnp->ifn_index = ifn_index;
		sctp_ifnp->ifn_p = ifn;
		sctp_ifnp->ifn_type = ifn_type;
		sctp_ifnp->refcount = 0;
		sctp_ifnp->vrf = vrf;
		atomic_add_int(&vrf->refcount, 1);
		sctp_ifnp->ifn_mtu = SCTP_GATHER_MTU_FROM_IFN_INFO(ifn, ifn_index, addr->sa_family);
		if (if_name != NULL) {
			snprintf(sctp_ifnp->ifn_name, SCTP_IFNAMSIZ, "%s", if_name);
		} else {
			snprintf(sctp_ifnp->ifn_name, SCTP_IFNAMSIZ, "%s", "unknown");
		}
		hash_ifn_head = &SCTP_BASE_INFO(vrf_ifn_hash)[(ifn_index & SCTP_BASE_INFO(vrf_ifn_hashmark))];
		LIST_INIT(&sctp_ifnp->ifalist);
		SCTP_IPI_ADDR_WLOCK();
		LIST_INSERT_HEAD(hash_ifn_head, sctp_ifnp, next_bucket);
		LIST_INSERT_HEAD(&vrf->ifnlist, sctp_ifnp, next_ifn);
		atomic_add_int(&SCTP_BASE_INFO(ipi_count_ifns), 1);
		new_ifn_af = 1;
	}
	sctp_ifap = sctp_find_ifa_by_addr(addr, vrf->vrf_id, SCTP_ADDR_LOCKED);
	if (sctp_ifap) {
		
		if ((sctp_ifap->ifn_p) &&
		    (sctp_ifap->ifn_p->ifn_index == ifn_index)) {
			SCTPDBG(SCTP_DEBUG_PCB4, "Using existing ifn %s (0x%x) for ifa %p\n",
				sctp_ifap->ifn_p->ifn_name, ifn_index,
				(void *)sctp_ifap);
			if (new_ifn_af) {
				
				sctp_delete_ifn(sctp_ifnp, SCTP_ADDR_LOCKED);
			}
			if (sctp_ifap->localifa_flags & SCTP_BEING_DELETED) {
				
				SCTPDBG(SCTP_DEBUG_PCB4, "Clearing deleted ifa flag\n");
				sctp_ifap->localifa_flags = SCTP_ADDR_VALID;
				sctp_ifap->ifn_p = sctp_ifnp;
				atomic_add_int(&sctp_ifap->ifn_p->refcount, 1);
			}
		exit_stage_left:
			SCTP_IPI_ADDR_WUNLOCK();
			return (sctp_ifap);
		} else {
			if (sctp_ifap->ifn_p) {
				



				SCTPDBG(SCTP_DEBUG_PCB4, "Moving ifa %p from %s (0x%x) to %s (0x%x)\n",
					(void *)sctp_ifap, sctp_ifap->ifn_p->ifn_name,
					sctp_ifap->ifn_p->ifn_index, if_name,
					ifn_index);
				
				sctp_remove_ifa_from_ifn(sctp_ifap);
				
				sctp_add_ifa_to_ifn(sctp_ifnp, sctp_ifap);
 				goto exit_stage_left;
			} else {
				
				sctp_ifap->localifa_flags = SCTP_ADDR_VALID;
				SCTPDBG(SCTP_DEBUG_PCB4, "Repairing ifn %p for ifa %p\n",
					(void *)sctp_ifnp, (void *)sctp_ifap);
				sctp_add_ifa_to_ifn(sctp_ifnp, sctp_ifap);
			}
			goto exit_stage_left;
		}
	}
	SCTP_IPI_ADDR_WUNLOCK();
	SCTP_MALLOC(sctp_ifap, struct sctp_ifa *, sizeof(struct sctp_ifa), SCTP_M_IFA);
	if (sctp_ifap == NULL) {
#ifdef INVARIANTS
		panic("No memory for IFA");
#endif
		return (NULL);
	}
	memset(sctp_ifap, 0, sizeof(struct sctp_ifa));
	sctp_ifap->ifn_p = sctp_ifnp;
	atomic_add_int(&sctp_ifnp->refcount, 1);
	sctp_ifap->vrf_id = vrf_id;
	sctp_ifap->ifa = ifa;
#ifdef HAVE_SA_LEN
	memcpy(&sctp_ifap->address, addr, addr->sa_len);
#else
	switch (addr->sa_family) {
#ifdef INET
	case AF_INET:
		memcpy(&sctp_ifap->address, addr, sizeof(struct sockaddr_in));
		break;
#endif
#ifdef INET6
	case AF_INET6:
		memcpy(&sctp_ifap->address, addr, sizeof(struct sockaddr_in6));
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		memcpy(&sctp_ifap->address, addr, sizeof(struct sockaddr_conn));
		break;
#endif
	default:
		
		break;
	}
#endif
	sctp_ifap->localifa_flags = SCTP_ADDR_VALID | SCTP_ADDR_DEFER_USE;
	sctp_ifap->flags = ifa_flags;
	
	switch (sctp_ifap->address.sa.sa_family) {
#ifdef INET
	case AF_INET:
	{
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)&sctp_ifap->address.sin;
		if (SCTP_IFN_IS_IFT_LOOP(sctp_ifap->ifn_p) ||
		    (IN4_ISLOOPBACK_ADDRESS(&sin->sin_addr))) {
			sctp_ifap->src_is_loop = 1;
		}
		if ((IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
			sctp_ifap->src_is_priv = 1;
		}
		sctp_ifnp->num_v4++;
		if (new_ifn_af)
		    new_ifn_af = AF_INET;
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)&sctp_ifap->address.sin6;
		if (SCTP_IFN_IS_IFT_LOOP(sctp_ifap->ifn_p) ||
		    (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))) {
			sctp_ifap->src_is_loop = 1;
		}
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
			sctp_ifap->src_is_priv = 1;
		}
		sctp_ifnp->num_v6++;
		if (new_ifn_af)
			new_ifn_af = AF_INET6;
		break;
	}
#endif
#if defined(__Userspace__)
	case AF_CONN:
		if (new_ifn_af)
			new_ifn_af = AF_CONN;
		break;
#endif
	default:
		new_ifn_af = 0;
		break;
	}
	hash_of_addr = sctp_get_ifa_hash_val(&sctp_ifap->address.sa);

	if ((sctp_ifap->src_is_priv == 0) &&
	    (sctp_ifap->src_is_loop == 0)) {
		sctp_ifap->src_is_glob = 1;
	}
	SCTP_IPI_ADDR_WLOCK();
	hash_addr_head = &vrf->vrf_addr_hash[(hash_of_addr & vrf->vrf_addr_hashmark)];
	LIST_INSERT_HEAD(hash_addr_head, sctp_ifap, next_bucket);
	sctp_ifap->refcount = 1;
	LIST_INSERT_HEAD(&sctp_ifnp->ifalist, sctp_ifap, next_ifa);
	sctp_ifnp->ifa_count++;
	vrf->total_ifa_count++;
	atomic_add_int(&SCTP_BASE_INFO(ipi_count_ifas), 1);
	if (new_ifn_af) {
		SCTP_REGISTER_INTERFACE(ifn_index, new_ifn_af);
		sctp_ifnp->registered_af = new_ifn_af;
	}
	SCTP_IPI_ADDR_WUNLOCK();
	if (dynamic_add) {
		


		struct sctp_laddr *wi;

		atomic_add_int(&sctp_ifap->refcount, 1);
		wi = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_laddr), struct sctp_laddr);
		if (wi == NULL) {
			



			SCTPDBG(SCTP_DEBUG_PCB4, "Lost an address change?\n");
			
			sctp_del_addr_from_vrf(vrf_id, addr, ifn_index,
					       if_name);
			return (NULL);
		}
		SCTP_INCR_LADDR_COUNT();
		bzero(wi, sizeof(*wi));
		(void)SCTP_GETTIME_TIMEVAL(&wi->start_time);
		wi->ifa = sctp_ifap;
		wi->action = SCTP_ADD_IP_ADDRESS;

		SCTP_WQ_ADDR_LOCK();
		LIST_INSERT_HEAD(&SCTP_BASE_INFO(addr_wq), wi, sctp_nxt_addr);
		SCTP_WQ_ADDR_UNLOCK();

		sctp_timer_start(SCTP_TIMER_TYPE_ADDR_WQ,
				 (struct sctp_inpcb *)NULL,
				 (struct sctp_tcb *)NULL,
				 (struct sctp_nets *)NULL);
	} else {
		
		sctp_ifap->localifa_flags &= ~SCTP_ADDR_DEFER_USE;
	}
	return (sctp_ifap);
}

void
sctp_del_addr_from_vrf(uint32_t vrf_id, struct sockaddr *addr,
		       uint32_t ifn_index, const char *if_name)
{
	struct sctp_vrf *vrf;
	struct sctp_ifa *sctp_ifap = NULL;

	SCTP_IPI_ADDR_WLOCK();
	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL) {
		SCTPDBG(SCTP_DEBUG_PCB4, "Can't find vrf_id 0x%x\n", vrf_id);
		goto out_now;
	}

#ifdef SCTP_DEBUG
	SCTPDBG(SCTP_DEBUG_PCB4, "vrf_id 0x%x: deleting address:", vrf_id);
	SCTPDBG_ADDR(SCTP_DEBUG_PCB4, addr);
#endif
	sctp_ifap = sctp_find_ifa_by_addr(addr, vrf->vrf_id, SCTP_ADDR_LOCKED);
	if (sctp_ifap) {
		
		if (sctp_ifap->ifn_p) {
			int valid = 0;
			




			if (if_name) {
				if (strncmp(if_name, sctp_ifap->ifn_p->ifn_name, SCTP_IFNAMSIZ) == 0) {
					
					valid = 1;
				}
			}
			if (!valid) {
				
				if (ifn_index == sctp_ifap->ifn_p->ifn_index) {
					valid = 1;
				}
			}
			if (!valid) {
				SCTPDBG(SCTP_DEBUG_PCB4, "ifn:%d ifname:%s does not match addresses\n",
					ifn_index, ((if_name == NULL) ? "NULL" : if_name));
				SCTPDBG(SCTP_DEBUG_PCB4, "ifn:%d ifname:%s - ignoring delete\n",
					sctp_ifap->ifn_p->ifn_index, sctp_ifap->ifn_p->ifn_name);
				SCTP_IPI_ADDR_WUNLOCK();
				return;
			}
		}
		SCTPDBG(SCTP_DEBUG_PCB4, "Deleting ifa %p\n", (void *)sctp_ifap);
		sctp_ifap->localifa_flags &= SCTP_ADDR_VALID;
		sctp_ifap->localifa_flags |= SCTP_BEING_DELETED;
		vrf->total_ifa_count--;
		LIST_REMOVE(sctp_ifap, next_bucket);
		sctp_remove_ifa_from_ifn(sctp_ifap);
	}
#ifdef SCTP_DEBUG
	else {
		SCTPDBG(SCTP_DEBUG_PCB4, "Del Addr-ifn:%d Could not find address:",
			ifn_index);
		SCTPDBG_ADDR(SCTP_DEBUG_PCB1, addr);
	}
#endif

 out_now:
	SCTP_IPI_ADDR_WUNLOCK();
	if (sctp_ifap) {
		struct sctp_laddr *wi;

		wi = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_laddr), struct sctp_laddr);
		if (wi == NULL) {
			



			SCTPDBG(SCTP_DEBUG_PCB4, "Lost an address change?\n");

			
			sctp_free_ifa(sctp_ifap);
			return;
		}
		SCTP_INCR_LADDR_COUNT();
		bzero(wi, sizeof(*wi));
		(void)SCTP_GETTIME_TIMEVAL(&wi->start_time);
		wi->ifa = sctp_ifap;
		wi->action = SCTP_DEL_IP_ADDRESS;
		SCTP_WQ_ADDR_LOCK();
		



		LIST_INSERT_HEAD(&SCTP_BASE_INFO(addr_wq), wi, sctp_nxt_addr);
		SCTP_WQ_ADDR_UNLOCK();

		sctp_timer_start(SCTP_TIMER_TYPE_ADDR_WQ,
				 (struct sctp_inpcb *)NULL,
				 (struct sctp_tcb *)NULL,
				 (struct sctp_nets *)NULL);
	}
	return;
}


static int
sctp_does_stcb_own_this_addr(struct sctp_tcb *stcb, struct sockaddr *to)
{
	int loopback_scope, ipv4_local_scope, local_scope, site_scope;
	int ipv4_addr_legal, ipv6_addr_legal;
#if defined(__Userspace__)
	int conn_addr_legal;
#endif
	struct sctp_vrf *vrf;
	struct sctp_ifn *sctp_ifn;
	struct sctp_ifa *sctp_ifa;

	loopback_scope = stcb->asoc.scope.loopback_scope;
	ipv4_local_scope = stcb->asoc.scope.ipv4_local_scope;
	local_scope = stcb->asoc.scope.local_scope;
	site_scope = stcb->asoc.scope.site_scope;
	ipv4_addr_legal = stcb->asoc.scope.ipv4_addr_legal;
	ipv6_addr_legal = stcb->asoc.scope.ipv6_addr_legal;
#if defined(__Userspace__)
	conn_addr_legal = stcb->asoc.scope.conn_addr_legal;
#endif

	SCTP_IPI_ADDR_RLOCK();
	vrf = sctp_find_vrf(stcb->asoc.vrf_id);
	if (vrf == NULL) {
		
		SCTP_IPI_ADDR_RUNLOCK();
		return (0);
	}

	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
			if ((loopback_scope == 0) &&
			    SCTP_IFN_IS_IFT_LOOP(sctp_ifn)) {
				continue;
			}
			LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
				if (sctp_is_addr_restricted(stcb, sctp_ifa) &&
				    (!sctp_is_addr_pending(stcb, sctp_ifa))) {
					



					continue;
				}
				
				switch (sctp_ifa->address.sa.sa_family) {
#ifdef INET
				case AF_INET:
					if (ipv4_addr_legal) {
						struct sockaddr_in *sin, *rsin;

						sin = &sctp_ifa->address.sin;
						rsin = (struct sockaddr_in *)to;
						if ((ipv4_local_scope == 0) &&
						    IN4_ISPRIVATE_ADDRESS(&sin->sin_addr)) {
							continue;
						}
						if (sin->sin_addr.s_addr == rsin->sin_addr.s_addr) {
							SCTP_IPI_ADDR_RUNLOCK();
							return (1);
						}
					}
					break;
#endif
#ifdef INET6
				case AF_INET6:
					if (ipv6_addr_legal) {
						struct sockaddr_in6 *sin6, *rsin6;
#if defined(SCTP_EMBEDDED_V6_SCOPE) && !defined(SCTP_KAME)
						struct sockaddr_in6 lsa6;
#endif
						sin6 = &sctp_ifa->address.sin6;
						rsin6 = (struct sockaddr_in6 *)to;
						if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
							if (local_scope == 0)
								continue;
#if defined(SCTP_EMBEDDED_V6_SCOPE)
							if (sin6->sin6_scope_id == 0) {
#ifdef SCTP_KAME
								if (sa6_recoverscope(sin6) != 0)
									continue;
#else
								lsa6 = *sin6;
								if (in6_recoverscope(&lsa6,
								                     &lsa6.sin6_addr,
								                     NULL))
									continue;
								sin6 = &lsa6;
#endif 
							}
#endif 
						}
						if ((site_scope == 0) &&
						    (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))) {
							continue;
						}
						if (SCTP6_ARE_ADDR_EQUAL(sin6, rsin6)) {
							SCTP_IPI_ADDR_RUNLOCK();
							return (1);
						}
					}
					break;
#endif
#if defined(__Userspace__)
				case AF_CONN:
					if (conn_addr_legal) {
						struct sockaddr_conn *sconn, *rsconn;

						sconn = &sctp_ifa->address.sconn;
						rsconn = (struct sockaddr_conn *)to;
						if (sconn->sconn_addr == rsconn->sconn_addr) {
							SCTP_IPI_ADDR_RUNLOCK();
							return (1);
						}
					}
					break;
#endif
				default:
					
					break;
				}
			}
		}
	} else {
		struct sctp_laddr *laddr;

		LIST_FOREACH(laddr, &stcb->sctp_ep->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED) {
				SCTPDBG(SCTP_DEBUG_PCB1, "ifa being deleted\n");
				continue;
			}
			if (sctp_is_addr_restricted(stcb, laddr->ifa) &&
			    (!sctp_is_addr_pending(stcb, laddr->ifa))) {
				



				continue;
			}
			if (laddr->ifa->address.sa.sa_family != to->sa_family) {
				continue;
			}
			switch (to->sa_family) {
#ifdef INET
			case AF_INET:
			{
				struct sockaddr_in *sin, *rsin;

				sin = (struct sockaddr_in *)&laddr->ifa->address.sin;
				rsin = (struct sockaddr_in *)to;
				if (sin->sin_addr.s_addr == rsin->sin_addr.s_addr) {
					SCTP_IPI_ADDR_RUNLOCK();
					return (1);
				}
				break;
			}
#endif
#ifdef INET6
			case AF_INET6:
			{
				struct sockaddr_in6 *sin6, *rsin6;

				sin6 = (struct sockaddr_in6 *)&laddr->ifa->address.sin6;
				rsin6 = (struct sockaddr_in6 *)to;
				if (SCTP6_ARE_ADDR_EQUAL(sin6, rsin6)) {
					SCTP_IPI_ADDR_RUNLOCK();
					return (1);
				}
				break;
			}

#endif
#if defined(__Userspace__)
			case AF_CONN:
			{
				struct sockaddr_conn *sconn, *rsconn;

				sconn = (struct sockaddr_conn *)&laddr->ifa->address.sconn;
				rsconn = (struct sockaddr_conn *)to;
				if (sconn->sconn_addr == rsconn->sconn_addr) {
					SCTP_IPI_ADDR_RUNLOCK();
					return (1);
				}
				break;
			}
#endif
			default:
				
				break;
			}

		}
	}
	SCTP_IPI_ADDR_RUNLOCK();
	return (0);
}


static struct sctp_tcb *
sctp_tcb_special_locate(struct sctp_inpcb **inp_p, struct sockaddr *from,
    struct sockaddr *to, struct sctp_nets **netp, uint32_t vrf_id)
{
	
	



	uint16_t lport, rport;
	struct sctppcbhead *ephead;
	struct sctp_inpcb *inp;
	struct sctp_laddr *laddr;
	struct sctp_tcb *stcb;
	struct sctp_nets *net;
#ifdef SCTP_MVRF
	int fnd, i;
#endif

	if ((to == NULL) || (from == NULL)) {
		return (NULL);
	}

	switch (to->sa_family) {
#ifdef INET
	case AF_INET:
		if (from->sa_family == AF_INET) {
			lport = ((struct sockaddr_in *)to)->sin_port;
			rport = ((struct sockaddr_in *)from)->sin_port;
		} else {
			return (NULL);
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		if (from->sa_family == AF_INET6) {
			lport = ((struct sockaddr_in6 *)to)->sin6_port;
			rport = ((struct sockaddr_in6 *)from)->sin6_port;
		} else {
			return (NULL);
		}
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		if (from->sa_family == AF_CONN) {
			lport = ((struct sockaddr_conn *)to)->sconn_port;
			rport = ((struct sockaddr_conn *)from)->sconn_port;
		} else {
			return (NULL);
		}
		break;
#endif
	default:
		return (NULL);
	}
	ephead = &SCTP_BASE_INFO(sctp_tcpephash)[SCTP_PCBHASH_ALLADDR((lport | rport), SCTP_BASE_INFO(hashtcpmark))];
	





	LIST_FOREACH(inp, ephead, sctp_hash) {
		SCTP_INP_RLOCK(inp);
		if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if (lport != inp->sctp_lport) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
#ifdef SCTP_MVRF
		fnd = 0;
		for (i = 0; i < inp->num_vrfs; i++) {
			if (inp->m_vrf_ids[i] == vrf_id) {
				fnd = 1;
				break;
			}
		}
		if (fnd == 0) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
#else
		if (inp->def_vrf_id != vrf_id) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
#endif
		
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0) {
			
			int match = 0;

			LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {

				if (laddr->ifa == NULL) {
					SCTPDBG(SCTP_DEBUG_PCB1, "%s: NULL ifa\n", __FUNCTION__);
					continue;
				}
				if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED) {
					SCTPDBG(SCTP_DEBUG_PCB1, "ifa being deleted\n");
					continue;
				}
				if (laddr->ifa->address.sa.sa_family ==
				    to->sa_family) {
					
#ifdef INET
					if (from->sa_family == AF_INET) {
						struct sockaddr_in *intf_addr, *sin;

						intf_addr = &laddr->ifa->address.sin;
						sin = (struct sockaddr_in *)to;
						if (sin->sin_addr.s_addr ==
						    intf_addr->sin_addr.s_addr) {
							match = 1;
							break;
						}
					}
#endif
#ifdef INET6
					if (from->sa_family == AF_INET6) {
						struct sockaddr_in6 *intf_addr6;
						struct sockaddr_in6 *sin6;

						sin6 = (struct sockaddr_in6 *)
						    to;
						intf_addr6 = &laddr->ifa->address.sin6;

						if (SCTP6_ARE_ADDR_EQUAL(sin6,
						    intf_addr6)) {
							match = 1;
							break;
						}
					}
#endif
#if defined(__Userspace__)
					if (from->sa_family == AF_CONN) {
						struct sockaddr_conn *intf_addr, *sconn;

						intf_addr = &laddr->ifa->address.sconn;
						sconn = (struct sockaddr_conn *)to;
						if (sconn->sconn_addr ==
						    intf_addr->sconn_addr) {
							match = 1;
							break;
						}
					}
#endif
				}
			}
			if (match == 0) {
				
				SCTP_INP_RUNLOCK(inp);
				continue;
			}
		}
		



		
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb == NULL) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		SCTP_TCB_LOCK(stcb);
		if (!sctp_does_stcb_own_this_addr(stcb, to)) {
			SCTP_TCB_UNLOCK(stcb);
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if (stcb->rport != rport) {
			
			SCTP_TCB_UNLOCK(stcb);
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
			SCTP_TCB_UNLOCK(stcb);
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if (!sctp_does_stcb_own_this_addr(stcb, to)) {
			SCTP_TCB_UNLOCK(stcb);
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		
		TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {

			if (net->ro._l_addr.sa.sa_family != from->sa_family) {
				
				continue;
			}
			switch (from->sa_family) {
#ifdef INET
			case AF_INET:
			{
				struct sockaddr_in *sin, *rsin;

				sin = (struct sockaddr_in *)&net->ro._l_addr;
				rsin = (struct sockaddr_in *)from;
				if (sin->sin_addr.s_addr ==
				    rsin->sin_addr.s_addr) {
					
					if (netp != NULL) {
						*netp = net;
					}
					
					*inp_p = inp;
					SCTP_INP_RUNLOCK(inp);
					return (stcb);
				}
				break;
			}
#endif
#ifdef INET6
			case AF_INET6:
			{
				struct sockaddr_in6 *sin6, *rsin6;

				sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
				rsin6 = (struct sockaddr_in6 *)from;
				if (SCTP6_ARE_ADDR_EQUAL(sin6,
				    rsin6)) {
					
					if (netp != NULL) {
						*netp = net;
					}
					
					*inp_p = inp;
					SCTP_INP_RUNLOCK(inp);
					return (stcb);
				}
				break;
			}
#endif
#if defined(__Userspace__)
			case AF_CONN:
			{
				struct sockaddr_conn *sconn, *rsconn;

				sconn = (struct sockaddr_conn *)&net->ro._l_addr;
				rsconn = (struct sockaddr_conn *)from;
				if (sconn->sconn_addr == rsconn->sconn_addr) {
					
					if (netp != NULL) {
						*netp = net;
					}
					
					*inp_p = inp;
					SCTP_INP_RUNLOCK(inp);
					return (stcb);
				}
				break;
			}
#endif
			default:
				
				break;
			}
		}
		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_RUNLOCK(inp);
	}
	return (NULL);
}











struct sctp_tcb *
sctp_findassociation_ep_addr(struct sctp_inpcb **inp_p, struct sockaddr *remote,
    struct sctp_nets **netp, struct sockaddr *local, struct sctp_tcb *locked_tcb)
{
	struct sctpasochead *head;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb = NULL;
	struct sctp_nets *net;
	uint16_t rport;

	inp = *inp_p;
	switch (remote->sa_family) {
#ifdef INET
	case AF_INET:
		rport = (((struct sockaddr_in *)remote)->sin_port);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		rport = (((struct sockaddr_in6 *)remote)->sin6_port);
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		rport = (((struct sockaddr_in6 *)remote)->sin6_port);
		break;
#endif
	default:
		return (NULL);
	}
	if (locked_tcb) {
		



		atomic_add_int(&locked_tcb->asoc.refcnt, 1);
		SCTP_TCB_UNLOCK(locked_tcb);
	}
	SCTP_INP_INFO_RLOCK();
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
		






		if ((inp->sctp_socket) && (inp->sctp_socket->so_qlimit)) {
			
#ifndef SCTP_MVRF
			stcb = sctp_tcb_special_locate(inp_p, remote, local,
			    netp, inp->def_vrf_id);
			if ((stcb != NULL) && (locked_tcb == NULL)) {
				
				SCTP_INP_DECR_REF(inp);
			}
			if ((locked_tcb != NULL) && (locked_tcb != stcb)) {
				SCTP_INP_RLOCK(locked_tcb->sctp_ep);
				SCTP_TCB_LOCK(locked_tcb);
				atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
				SCTP_INP_RUNLOCK(locked_tcb->sctp_ep);
			}
#else
			



			int i;

			for (i = 0; i < inp->num_vrfs; i++) {
				stcb = sctp_tcb_special_locate(inp_p, remote, local,
				                               netp, inp->m_vrf_ids[i]);
				if ((stcb != NULL) && (locked_tcb == NULL)) {
					
					SCTP_INP_DECR_REF(inp);
					break;
				}
				if ((locked_tcb != NULL) && (locked_tcb != stcb)) {
					SCTP_INP_RLOCK(locked_tcb->sctp_ep);
					SCTP_TCB_LOCK(locked_tcb);
					atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
					SCTP_INP_RUNLOCK(locked_tcb->sctp_ep);
					break;
				}
			}
#endif
			SCTP_INP_INFO_RUNLOCK();
			return (stcb);
		} else {
			SCTP_INP_WLOCK(inp);
			if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
				goto null_return;
			}
			stcb = LIST_FIRST(&inp->sctp_asoc_list);
			if (stcb == NULL) {
				goto null_return;
			}
			SCTP_TCB_LOCK(stcb);

			if (stcb->rport != rport) {
				
				SCTP_TCB_UNLOCK(stcb);
				goto null_return;
			}
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				SCTP_TCB_UNLOCK(stcb);
				goto null_return;
			}
			if (local && !sctp_does_stcb_own_this_addr(stcb, local)) {
				SCTP_TCB_UNLOCK(stcb);
				goto null_return;
			}
			
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
#ifdef INVARIANTS
				if (net == (TAILQ_NEXT(net, sctp_next))) {
					panic("Corrupt net list");
				}
#endif
				if (net->ro._l_addr.sa.sa_family !=
				    remote->sa_family) {
					
					continue;
				}
				switch (remote->sa_family) {
#ifdef INET
				case AF_INET:
				{
					struct sockaddr_in *sin, *rsin;

					sin = (struct sockaddr_in *)
					    &net->ro._l_addr;
					rsin = (struct sockaddr_in *)remote;
					if (sin->sin_addr.s_addr ==
					    rsin->sin_addr.s_addr) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}

						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
#ifdef INET6
				case AF_INET6:
				{
					struct sockaddr_in6 *sin6, *rsin6;

					sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
					rsin6 = (struct sockaddr_in6 *)remote;
					if (SCTP6_ARE_ADDR_EQUAL(sin6,
					    rsin6)) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}
						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
#if defined(__Userspace__)
				case AF_CONN:
				{
					struct sockaddr_conn *sconn, *rsconn;

					sconn = (struct sockaddr_conn *)&net->ro._l_addr;
					rsconn = (struct sockaddr_conn *)remote;
					if (sconn->sconn_addr == rsconn->sconn_addr) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}
						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
				default:
					
					break;
				}
			}
			SCTP_TCB_UNLOCK(stcb);
		}
	} else {
		SCTP_INP_WLOCK(inp);
		if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			goto null_return;
		}
		head = &inp->sctp_tcbhash[SCTP_PCBHASH_ALLADDR(rport,
		    inp->sctp_hashmark)];
		if (head == NULL) {
			goto null_return;
		}
		LIST_FOREACH(stcb, head, sctp_tcbhash) {
			if (stcb->rport != rport) {
				
				continue;
			}
			SCTP_TCB_LOCK(stcb);
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			if (local && !sctp_does_stcb_own_this_addr(stcb, local)) {
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
#ifdef INVARIANTS
				if (net == (TAILQ_NEXT(net, sctp_next))) {
					panic("Corrupt net list");
				}
#endif
				if (net->ro._l_addr.sa.sa_family !=
				    remote->sa_family) {
					
					continue;
				}
				switch (remote->sa_family) {
#ifdef INET
				case AF_INET:
				{
					struct sockaddr_in *sin, *rsin;

					sin = (struct sockaddr_in *)
					    &net->ro._l_addr;
					rsin = (struct sockaddr_in *)remote;
					if (sin->sin_addr.s_addr ==
					    rsin->sin_addr.s_addr) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}
						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
#ifdef INET6
				case AF_INET6:
				{
					struct sockaddr_in6 *sin6, *rsin6;

					sin6 = (struct sockaddr_in6 *)
					    &net->ro._l_addr;
					rsin6 = (struct sockaddr_in6 *)remote;
					if (SCTP6_ARE_ADDR_EQUAL(sin6,
					    rsin6)) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}
						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
#if defined(__Userspace__)
				case AF_CONN:
				{
					struct sockaddr_conn *sconn, *rsconn;

					sconn = (struct sockaddr_conn *)&net->ro._l_addr;
					rsconn = (struct sockaddr_conn *)remote;
					if (sconn->sconn_addr == rsconn->sconn_addr) {
						
						if (netp != NULL) {
							*netp = net;
						}
						if (locked_tcb == NULL) {
							SCTP_INP_DECR_REF(inp);
						} else if (locked_tcb != stcb) {
							SCTP_TCB_LOCK(locked_tcb);
						}
						if (locked_tcb) {
							atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
						}
						SCTP_INP_WUNLOCK(inp);
						SCTP_INP_INFO_RUNLOCK();
						return (stcb);
					}
					break;
				}
#endif
				default:
					
					break;
				}
			}
			SCTP_TCB_UNLOCK(stcb);
		}
	}
null_return:
	
	if (locked_tcb) {
		SCTP_TCB_LOCK(locked_tcb);
		atomic_subtract_int(&locked_tcb->asoc.refcnt, 1);
	}
	SCTP_INP_WUNLOCK(inp);
	SCTP_INP_INFO_RUNLOCK();
	
	return (NULL);
}






struct sctp_tcb *
sctp_findasoc_ep_asocid_locked(struct sctp_inpcb *inp, sctp_assoc_t asoc_id, int want_lock)
{
	


	struct sctpasochead *head;
	struct sctp_tcb *stcb;
	uint32_t id;

	if (inp == NULL) {
		SCTP_PRINTF("TSNH ep_associd\n");
		return (NULL);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
		SCTP_PRINTF("TSNH ep_associd0\n");
		return (NULL);
	}
	id = (uint32_t)asoc_id;
	head = &inp->sctp_asocidhash[SCTP_PCBHASH_ASOC(id, inp->hashasocidmark)];
	if (head == NULL) {
		
		SCTP_PRINTF("TSNH ep_associd1\n");
		return (NULL);
	}
	LIST_FOREACH(stcb, head, sctp_tcbasocidhash) {
		if (stcb->asoc.assoc_id == id) {
			if (inp != stcb->sctp_ep) {
				



				SCTP_PRINTF("TSNH ep_associd2\n");
				continue;
			}
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				continue;
			}
			if (want_lock) {
				SCTP_TCB_LOCK(stcb);
			}
			return (stcb);
		}
	}
	return (NULL);
}


struct sctp_tcb *
sctp_findassociation_ep_asocid(struct sctp_inpcb *inp, sctp_assoc_t asoc_id, int want_lock)
{
	struct sctp_tcb *stcb;

	SCTP_INP_RLOCK(inp);
	stcb = sctp_findasoc_ep_asocid_locked(inp, asoc_id, want_lock);
	SCTP_INP_RUNLOCK(inp);
	return (stcb);
}





static struct sctp_inpcb *
sctp_endpoint_probe(struct sockaddr *nam, struct sctppcbhead *head,
		    uint16_t lport, uint32_t vrf_id)
{
	struct sctp_inpcb *inp;
	struct sctp_laddr *laddr;
#ifdef INET
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6;
	struct sockaddr_in6 *intf_addr6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn *sconn;
#endif
#ifdef SCTP_MVRF
	int i;
#endif
	int  fnd;

#ifdef INET
	sin = NULL;
#endif
#ifdef INET6
	sin6 = NULL;
#endif
#if defined(__Userspace__)
	sconn = NULL;
#endif
	switch (nam->sa_family) {
#ifdef INET
	case AF_INET:
		sin = (struct sockaddr_in *)nam;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)nam;
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		sconn = (struct sockaddr_conn *)nam;
		break;
#endif
	default:
		
		return (NULL);
	}

	if (head == NULL)
		return (NULL);

	LIST_FOREACH(inp, head, sctp_hash) {
		SCTP_INP_RLOCK(inp);
		if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) &&
		    (inp->sctp_lport == lport)) {
			
#ifdef INET
			if ((nam->sa_family == AF_INET) &&
			    (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
			    SCTP_IPV6_V6ONLY(inp)) {
				
				SCTP_INP_RUNLOCK(inp);
				continue;
			}
#endif
#ifdef INET6
			
			if (nam->sa_family == AF_INET6 &&
			    (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0) {
				SCTP_INP_RUNLOCK(inp);
				continue;
			}
#endif
			
			fnd = 0;
#ifdef SCTP_MVRF
			for (i = 0; i < inp->num_vrfs; i++) {
				if (inp->m_vrf_ids[i] == vrf_id) {
					fnd = 1;
					break;
				}
			}
#else
			if (inp->def_vrf_id == vrf_id)
				fnd = 1;
#endif

			SCTP_INP_RUNLOCK(inp);
			if (!fnd)
				continue;
			return (inp);
		}
		SCTP_INP_RUNLOCK(inp);
	}
	switch (nam->sa_family) {
#ifdef INET
	case AF_INET:
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			
			return (NULL);
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			
			return (NULL);
		}
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		if (sconn->sconn_addr == NULL) {
			return (NULL);
		}
		break;
#endif
	default:
		break;
	}
	



	LIST_FOREACH(inp, head, sctp_hash) {
		SCTP_INP_RLOCK(inp);
		if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL)) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		



		if (inp->sctp_lport != lport) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		
		fnd = 0;
#ifdef SCTP_MVRF
		for (i = 0; i < inp->num_vrfs; i++) {
			if (inp->m_vrf_ids[i] == vrf_id) {
				fnd = 1;
				break;
			}
		}
#else
		if (inp->def_vrf_id == vrf_id)
			fnd = 1;

#endif
		if (!fnd) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa == NULL) {
				SCTPDBG(SCTP_DEBUG_PCB1, "%s: NULL ifa\n",
					__FUNCTION__);
				continue;
			}
			SCTPDBG(SCTP_DEBUG_PCB1, "Ok laddr->ifa:%p is possible, ",
				(void *)laddr->ifa);
			if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED) {
				SCTPDBG(SCTP_DEBUG_PCB1, "Huh IFA being deleted\n");
				continue;
			}
			if (laddr->ifa->address.sa.sa_family == nam->sa_family) {
				
				switch (nam->sa_family) {
#ifdef INET
				case AF_INET:
#if defined(__APPLE__)
					if (sin == NULL) {
						
						break;
					}
#endif
					if (sin->sin_addr.s_addr ==
					    laddr->ifa->address.sin.sin_addr.s_addr) {
						SCTP_INP_RUNLOCK(inp);
						return (inp);
					}
					break;
#endif
#ifdef INET6
				case AF_INET6:
					intf_addr6 = &laddr->ifa->address.sin6;
					if (SCTP6_ARE_ADDR_EQUAL(sin6,
					    intf_addr6)) {
						SCTP_INP_RUNLOCK(inp);
						return (inp);
					}
					break;
#endif
#if defined(__Userspace__)
				case AF_CONN:
					if (sconn->sconn_addr == laddr->ifa->address.sconn.sconn_addr) {
						SCTP_INP_RUNLOCK(inp);
						return (inp);
					}
					break;
#endif
				}
			}
		}
		SCTP_INP_RUNLOCK(inp);
	}
	return (NULL);
}


static struct sctp_inpcb *
sctp_isport_inuse(struct sctp_inpcb *inp, uint16_t lport, uint32_t vrf_id)
{
	struct sctppcbhead *head;
	struct sctp_inpcb *t_inp;
#ifdef SCTP_MVRF
	int i;
#endif
	int fnd;

	head = &SCTP_BASE_INFO(sctp_ephash)[SCTP_PCBHASH_ALLADDR(lport,
	    SCTP_BASE_INFO(hashmark))];
	LIST_FOREACH(t_inp, head, sctp_hash) {
		if (t_inp->sctp_lport != lport) {
			continue;
		}
		
		fnd = 0;
#ifdef SCTP_MVRF
		for (i = 0; i < inp->num_vrfs; i++) {
			if (t_inp->m_vrf_ids[i] == vrf_id) {
				fnd = 1;
				break;
			}
		}
#else
		if (t_inp->def_vrf_id == vrf_id)
			fnd = 1;
#endif
		if (!fnd)
			continue;

		
		
		if ((t_inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
		    SCTP_IPV6_V6ONLY(t_inp)) {
			if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
				
				return (t_inp);
			} else {
				
				continue;
			}
		} else if (t_inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
			
			return (t_inp);
		} else {
			
			if ((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) &&
			    SCTP_IPV6_V6ONLY(inp)) {
				
				continue;
			}
			
		}
		return (t_inp);
	}
	return (NULL);
}


int
sctp_swap_inpcb_for_listen(struct sctp_inpcb *inp)
{
	
	struct sctppcbhead *head;
	struct sctp_inpcb *tinp;

	if (sctp_is_feature_off(inp, SCTP_PCB_FLAGS_PORTREUSE)) {
		
		return (-1);
	}
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) == 0) {
		return (0);
	}
	SCTP_INP_RUNLOCK(inp);
	head = &SCTP_BASE_INFO(sctp_ephash)[SCTP_PCBHASH_ALLADDR(inp->sctp_lport,
	                                    SCTP_BASE_INFO(hashmark))];
	
	LIST_FOREACH(tinp, head, sctp_hash) {
		if (tinp->sctp_lport != inp->sctp_lport) {
			continue;
		}
		if (tinp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			continue;
		}
		if (tinp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
			continue;
		}
		if (tinp->sctp_socket->so_qlimit) {
			continue;
		}
		SCTP_INP_WLOCK(tinp);
		LIST_REMOVE(tinp, sctp_hash);
		head = &SCTP_BASE_INFO(sctp_tcpephash)[SCTP_PCBHASH_ALLADDR(tinp->sctp_lport, SCTP_BASE_INFO(hashtcpmark))];
		tinp->sctp_flags |= SCTP_PCB_FLAGS_IN_TCPPOOL;
		LIST_INSERT_HEAD(head, tinp, sctp_hash);
		SCTP_INP_WUNLOCK(tinp);
	}
	SCTP_INP_WLOCK(inp);
	
	LIST_REMOVE(inp, sctp_hash);
	inp->sctp_flags &= ~SCTP_PCB_FLAGS_IN_TCPPOOL;
	head = &SCTP_BASE_INFO(sctp_ephash)[SCTP_PCBHASH_ALLADDR(inp->sctp_lport, SCTP_BASE_INFO(hashmark))];
	LIST_INSERT_HEAD(head, inp, sctp_hash);
	SCTP_INP_WUNLOCK(inp);
	SCTP_INP_RLOCK(inp);
	return (0);
}


struct sctp_inpcb *
sctp_pcb_findep(struct sockaddr *nam, int find_tcp_pool, int have_lock,
		uint32_t vrf_id)
{
	



	struct sctp_inpcb *inp;
	struct sctppcbhead *head;
	int lport;
	unsigned int i;
#ifdef INET
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn *sconn;
#endif

	switch (nam->sa_family) {
#ifdef INET
	case AF_INET:
		sin = (struct sockaddr_in *)nam;
		lport = sin->sin_port;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)nam;
		lport = sin6->sin6_port;
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		sconn = (struct sockaddr_conn *)nam;
		lport = sconn->sconn_port;
		break;
#endif
	default:
		return (NULL);
	}
	




	
	if (have_lock == 0) {
		SCTP_INP_INFO_RLOCK();
	}
	head = &SCTP_BASE_INFO(sctp_ephash)[SCTP_PCBHASH_ALLADDR(lport,
	    SCTP_BASE_INFO(hashmark))];
	inp = sctp_endpoint_probe(nam, head, lport, vrf_id);

	









	if (inp == NULL && find_tcp_pool) {
		for (i = 0; i < SCTP_BASE_INFO(hashtcpmark) + 1; i++) {
			head = &SCTP_BASE_INFO(sctp_tcpephash)[i];
			inp = sctp_endpoint_probe(nam, head, lport, vrf_id);
			if (inp) {
				break;
			}
		}
	}
	if (inp) {
		SCTP_INP_INCR_REF(inp);
	}
	if (have_lock == 0) {
		SCTP_INP_INFO_RUNLOCK();
	}
	return (inp);
}







struct sctp_tcb *
sctp_findassociation_addr_sa(struct sockaddr *from, struct sockaddr *to,
    struct sctp_inpcb **inp_p, struct sctp_nets **netp, int find_tcp_pool,
    uint32_t vrf_id)
{
	struct sctp_inpcb *inp = NULL;
	struct sctp_tcb *stcb;

	SCTP_INP_INFO_RLOCK();
	if (find_tcp_pool) {
		if (inp_p != NULL) {
			stcb = sctp_tcb_special_locate(inp_p, from, to, netp,
			                               vrf_id);
		} else {
			stcb = sctp_tcb_special_locate(&inp, from, to, netp,
			                               vrf_id);
		}
		if (stcb != NULL) {
			SCTP_INP_INFO_RUNLOCK();
			return (stcb);
		}
	}
	inp = sctp_pcb_findep(to, 0, 1, vrf_id);
	if (inp_p != NULL) {
		*inp_p = inp;
	}
	SCTP_INP_INFO_RUNLOCK();
	if (inp == NULL) {
		return (NULL);
	}
	





	if (inp_p != NULL) {
		stcb = sctp_findassociation_ep_addr(inp_p, from, netp, to,
		                                    NULL);
	} else {
		stcb = sctp_findassociation_ep_addr(&inp, from, netp, to,
		                                    NULL);
	}
	return (stcb);
}







static struct sctp_tcb *
sctp_findassociation_special_addr(struct mbuf *m, int offset,
    struct sctphdr *sh, struct sctp_inpcb **inp_p, struct sctp_nets **netp,
    struct sockaddr *dst)
{
	struct sctp_paramhdr *phdr, parm_buf;
	struct sctp_tcb *stcb;
	uint32_t ptype, plen;
#ifdef INET
	struct sockaddr_in sin4;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6;
#endif

#ifdef INET
	memset(&sin4, 0, sizeof(sin4));
#ifdef HAVE_SIN_LEN
	sin4.sin_len = sizeof(sin4);
#endif
	sin4.sin_family = AF_INET;
	sin4.sin_port = sh->src_port;
#endif
#ifdef INET6
	memset(&sin6, 0, sizeof(sin6));
#ifdef HAVE_SIN6_LEN
	sin6.sin6_len = sizeof(sin6);
#endif
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = sh->src_port;
#endif

	stcb = NULL;
	offset += sizeof(struct sctp_init_chunk);

	phdr = sctp_get_next_param(m, offset, &parm_buf, sizeof(parm_buf));
	while (phdr != NULL) {
		
		ptype = ntohs(phdr->param_type);
		plen = ntohs(phdr->param_length);
		if (plen == 0) {
			break;
		}
#ifdef INET
		if (ptype == SCTP_IPV4_ADDRESS &&
		    plen == sizeof(struct sctp_ipv4addr_param)) {
			
			struct sctp_ipv4addr_param ip4_parm, *p4;

			phdr = sctp_get_next_param(m, offset,
			    (struct sctp_paramhdr *)&ip4_parm, min(plen, sizeof(ip4_parm)));
			if (phdr == NULL) {
				return (NULL);
			}
			p4 = (struct sctp_ipv4addr_param *)phdr;
			memcpy(&sin4.sin_addr, &p4->addr, sizeof(p4->addr));
			
			stcb = sctp_findassociation_ep_addr(inp_p,
			    (struct sockaddr *)&sin4, netp, dst, NULL);
			if (stcb != NULL) {
				return (stcb);
			}
		}
#endif
#ifdef INET6
		if (ptype == SCTP_IPV6_ADDRESS &&
		    plen == sizeof(struct sctp_ipv6addr_param)) {
			
			struct sctp_ipv6addr_param ip6_parm, *p6;

			phdr = sctp_get_next_param(m, offset,
			    (struct sctp_paramhdr *)&ip6_parm, min(plen,sizeof(ip6_parm)));
			if (phdr == NULL) {
				return (NULL);
			}
			p6 = (struct sctp_ipv6addr_param *)phdr;
			memcpy(&sin6.sin6_addr, &p6->addr, sizeof(p6->addr));
			
			stcb = sctp_findassociation_ep_addr(inp_p,
			    (struct sockaddr *)&sin6, netp, dst, NULL);
			if (stcb != NULL) {
				return (stcb);
			}
		}
#endif
		offset += SCTP_SIZE32(plen);
		phdr = sctp_get_next_param(m, offset, &parm_buf,
					   sizeof(parm_buf));
	}
	return (NULL);
}

static struct sctp_tcb *
sctp_findassoc_by_vtag(struct sockaddr *from, struct sockaddr *to, uint32_t vtag,
		       struct sctp_inpcb **inp_p, struct sctp_nets **netp, uint16_t rport,
		       uint16_t lport, int skip_src_check, uint32_t vrf_id, uint32_t remote_tag)
{
	




	struct sctpasochead *head;
	struct sctp_nets *net;
	struct sctp_tcb *stcb;
#ifdef SCTP_MVRF
	unsigned int i;
#endif

	SCTP_INP_INFO_RLOCK();
	head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(vtag,
	    SCTP_BASE_INFO(hashasocmark))];
	if (head == NULL) {
		
		SCTP_INP_INFO_RUNLOCK();
		return (NULL);
	}
	LIST_FOREACH(stcb, head, sctp_asocs) {
		SCTP_INP_RLOCK(stcb->sctp_ep);
		if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			SCTP_INP_RUNLOCK(stcb->sctp_ep);
			continue;
		}
#ifdef SCTP_MVRF
		for (i = 0; i < stcb->sctp_ep->num_vrfs; i++) {
			if (stcb->sctp_ep->m_vrf_ids[i] == vrf_id) {
				break;
			}
		}
		if (i == stcb->sctp_ep->num_vrfs) {
			SCTP_INP_RUNLOCK(inp);
			continue;
		}
#else
		if (stcb->sctp_ep->def_vrf_id != vrf_id) {
			SCTP_INP_RUNLOCK(stcb->sctp_ep);
			continue;
		}
#endif
		SCTP_TCB_LOCK(stcb);
		SCTP_INP_RUNLOCK(stcb->sctp_ep);
		if (stcb->asoc.my_vtag == vtag) {
			
			if (stcb->rport != rport) {
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			if (stcb->sctp_ep->sctp_lport != lport) {
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			
			if (sctp_does_stcb_own_this_addr(stcb, to) == 0) {
			        
				SCTP_TCB_UNLOCK(stcb);
				continue;
			}
			if (remote_tag) {
				
				if (stcb->asoc.peer_vtag == remote_tag) {
					


					goto conclusive;
				}
			}
			if (skip_src_check) {
			conclusive:
			        if (from) {
					*netp = sctp_findnet(stcb, from);
				} else {
					*netp = NULL;	
				}
				if (inp_p)
					*inp_p = stcb->sctp_ep;
				SCTP_INP_INFO_RUNLOCK();
				return (stcb);
			}
			net = sctp_findnet(stcb, from);
			if (net) {
				
				*netp = net;
				SCTP_STAT_INCR(sctps_vtagexpress);
				*inp_p = stcb->sctp_ep;
				SCTP_INP_INFO_RUNLOCK();
				return (stcb);
			} else {
				



				SCTP_STAT_INCR(sctps_vtagbogus);
			}
		}
		SCTP_TCB_UNLOCK(stcb);
	}
	SCTP_INP_INFO_RUNLOCK();
	return (NULL);
}






struct sctp_tcb *
sctp_findassociation_addr(struct mbuf *m, int offset,
    struct sockaddr *src, struct sockaddr *dst,
    struct sctphdr *sh, struct sctp_chunkhdr *ch,
    struct sctp_inpcb **inp_p, struct sctp_nets **netp, uint32_t vrf_id)
{
	int find_tcp_pool;
	struct sctp_tcb *stcb;
	struct sctp_inpcb *inp;

	if (sh->v_tag) {
		
		stcb = sctp_findassoc_by_vtag(src, dst, ntohl(sh->v_tag),
		                                inp_p, netp, sh->src_port, sh->dest_port, 0, vrf_id, 0);
		if (stcb) {
			return (stcb);
		}
	}

	find_tcp_pool = 0;
	if ((ch->chunk_type != SCTP_INITIATION) &&
	    (ch->chunk_type != SCTP_INITIATION_ACK) &&
	    (ch->chunk_type != SCTP_COOKIE_ACK) &&
	    (ch->chunk_type != SCTP_COOKIE_ECHO)) {
		
		find_tcp_pool = 1;
	}
	if (inp_p) {
		stcb = sctp_findassociation_addr_sa(src, dst, inp_p, netp,
		    find_tcp_pool, vrf_id);
		inp = *inp_p;
	} else {
		stcb = sctp_findassociation_addr_sa(src, dst, &inp, netp,
		    find_tcp_pool, vrf_id);
	}
	SCTPDBG(SCTP_DEBUG_PCB1, "stcb:%p inp:%p\n", (void *)stcb, (void *)inp);
	if (stcb == NULL && inp) {
		
		if ((ch->chunk_type == SCTP_INITIATION) ||
		    (ch->chunk_type == SCTP_INITIATION_ACK)) {
			







			if (inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) {
				if (inp_p) {
					*inp_p = NULL;
				}
				return (NULL);
			}
			stcb = sctp_findassociation_special_addr(m,
			    offset, sh, &inp, netp, dst);
			if (inp_p != NULL) {
				*inp_p = inp;
			}
		}
	}
	SCTPDBG(SCTP_DEBUG_PCB1, "stcb is %p\n", (void *)stcb);
	return (stcb);
}





struct sctp_tcb *
sctp_findassociation_ep_asconf(struct mbuf *m, int offset,
			       struct sockaddr *dst, struct sctphdr *sh,
                               struct sctp_inpcb **inp_p, struct sctp_nets **netp, uint32_t vrf_id)
{
	struct sctp_tcb *stcb;
	struct sockaddr_storage remote_store;
	struct sctp_paramhdr parm_buf, *phdr;
	int ptype;
	int zero_address = 0;
#ifdef INET
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif

	memset(&remote_store, 0, sizeof(remote_store));
	phdr = sctp_get_next_param(m, offset + sizeof(struct sctp_asconf_chunk),
				   &parm_buf, sizeof(struct sctp_paramhdr));
	if (phdr == NULL) {
		SCTPDBG(SCTP_DEBUG_INPUT3, "%s: failed to get asconf lookup addr\n",
			__FUNCTION__);
		return NULL;
	}
	ptype = (int)((uint32_t) ntohs(phdr->param_type));
	
	switch (ptype) {
#ifdef INET6
	case SCTP_IPV6_ADDRESS:
	{
		
		struct sctp_ipv6addr_param *p6, p6_buf;

		if (ntohs(phdr->param_length) != sizeof(struct sctp_ipv6addr_param)) {
			return NULL;
		}
		p6 = (struct sctp_ipv6addr_param *)sctp_get_next_param(m,
								       offset + sizeof(struct sctp_asconf_chunk),
								       &p6_buf.ph, sizeof(*p6));
		if (p6 == NULL) {
			SCTPDBG(SCTP_DEBUG_INPUT3, "%s: failed to get asconf v6 lookup addr\n",
				__FUNCTION__);
			return (NULL);
		}
		sin6 = (struct sockaddr_in6 *)&remote_store;
		sin6->sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		sin6->sin6_len = sizeof(*sin6);
#endif
		sin6->sin6_port = sh->src_port;
		memcpy(&sin6->sin6_addr, &p6->addr, sizeof(struct in6_addr));
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
			zero_address = 1;
		break;
	}
#endif
#ifdef INET
	case SCTP_IPV4_ADDRESS:
	{
		
		struct sctp_ipv4addr_param *p4, p4_buf;

		if (ntohs(phdr->param_length) != sizeof(struct sctp_ipv4addr_param)) {
			return NULL;
		}
		p4 = (struct sctp_ipv4addr_param *)sctp_get_next_param(m,
								       offset + sizeof(struct sctp_asconf_chunk),
								       &p4_buf.ph, sizeof(*p4));
		if (p4 == NULL) {
			SCTPDBG(SCTP_DEBUG_INPUT3, "%s: failed to get asconf v4 lookup addr\n",
				__FUNCTION__);
			return (NULL);
		}
		sin = (struct sockaddr_in *)&remote_store;
		sin->sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
		sin->sin_len = sizeof(*sin);
#endif
		sin->sin_port = sh->src_port;
		memcpy(&sin->sin_addr, &p4->addr, sizeof(struct in_addr));
		if (sin->sin_addr.s_addr == INADDR_ANY)
			zero_address = 1;
		break;
	}
#endif
	default:
		
		return NULL;
	}

	if (zero_address) {
	        stcb = sctp_findassoc_by_vtag(NULL, dst, ntohl(sh->v_tag), inp_p,
					      netp, sh->src_port, sh->dest_port, 1, vrf_id, 0);
		if (stcb != NULL) {
			SCTP_INP_DECR_REF(*inp_p);
		}
	} else {
		stcb = sctp_findassociation_ep_addr(inp_p,
		    (struct sockaddr *)&remote_store, netp,
		    dst, NULL);
	}
	return (stcb);
}







int
sctp_inpcb_alloc(struct socket *so, uint32_t vrf_id)
{
	







	int i, error;
	struct sctp_inpcb *inp;
	struct sctp_pcb *m;
	struct timeval time;
	sctp_sharedkey_t *null_key;

	error = 0;

	SCTP_INP_INFO_WLOCK();
	inp = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_ep), struct sctp_inpcb);
	if (inp == NULL) {
		SCTP_PRINTF("Out of SCTP-INPCB structures - no resources\n");
		SCTP_INP_INFO_WUNLOCK();
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOBUFS);
		return (ENOBUFS);
	}
	
	bzero(inp, sizeof(*inp));

	
#if defined(__APPLE__)
	inp->ip_inp.inp.inp_state = INPCB_STATE_INUSE;
#endif
	
	inp->sctp_socket = so;
	inp->ip_inp.inp.inp_socket = so;
#ifdef INET6
#if !defined(__Userspace__) && !defined(__Windows__)
	if (MODULE_GLOBAL(ip6_auto_flowlabel)) {
		inp->ip_inp.inp.inp_flags |= IN6P_AUTOFLOWLABEL;
	}
#endif
#endif
	inp->sctp_associd_counter = 1;
	inp->partial_delivery_point = SCTP_SB_LIMIT_RCV(so) >> SCTP_PARTIAL_DELIVERY_SHIFT;
	inp->sctp_frag_point = SCTP_DEFAULT_MAXSEGMENT;
	inp->sctp_cmt_on_off = SCTP_BASE_SYSCTL(sctp_cmt_on_off);
	inp->sctp_ecn_enable = SCTP_BASE_SYSCTL(sctp_ecn_enable);
#if defined(__Userspace__)
	inp->ulp_info = NULL;
	inp->recv_callback = NULL;
	inp->send_callback = NULL;
	inp->send_sb_threshold = 0;
#endif
	
	inp->sctp_asocidhash = SCTP_HASH_INIT(SCTP_STACK_VTAG_HASH_SIZE, &inp->hashasocidmark);
	if (inp->sctp_asocidhash == NULL) {
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		SCTP_INP_INFO_WUNLOCK();
		return (ENOBUFS);
	}
#ifdef IPSEC
#if !(defined(__APPLE__))
	{
		struct inpcbpolicy *pcb_sp = NULL;

		error = ipsec_init_policy(so, &pcb_sp);
		
		inp->ip_inp.inp.inp_sp = pcb_sp;
		((struct in6pcb *)(&inp->ip_inp.inp))->in6p_sp = pcb_sp;
	}
#else
	
	error = 0;
#endif
	if (error != 0) {
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		SCTP_INP_INFO_WUNLOCK();
		return error;
	}
#endif				
	SCTP_INCR_EP_COUNT();
	inp->ip_inp.inp.inp_ip_ttl = MODULE_GLOBAL(ip_defttl);
	SCTP_INP_INFO_WUNLOCK();

	so->so_pcb = (caddr_t)inp;

#if defined(__FreeBSD__) && __FreeBSD_version < 803000
	if ((SCTP_SO_TYPE(so) == SOCK_DGRAM) ||
	    (SCTP_SO_TYPE(so) == SOCK_SEQPACKET)) {
#else
	if (SCTP_SO_TYPE(so) == SOCK_SEQPACKET) {
#endif

		inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
		    SCTP_PCB_FLAGS_UNBOUND);
		
		
	} else if (SCTP_SO_TYPE(so) == SOCK_STREAM) {
		
		inp->sctp_flags = (SCTP_PCB_FLAGS_TCPTYPE |
		    SCTP_PCB_FLAGS_UNBOUND);
		
		SCTP_CLEAR_SO_NBIO(so);
#if defined(__Panda__)
	} else if (SCTP_SO_TYPE(so) == SOCK_FASTSEQPACKET) {
		inp->sctp_flags = (SCTP_PCB_FLAGS_UDPTYPE |
		    SCTP_PCB_FLAGS_UNBOUND);
		sctp_feature_on(inp, SCTP_PCB_FLAGS_ZERO_COPY_ACTIVE);
	} else if (SCTP_SO_TYPE(so) == SOCK_FASTSTREAM) {
		inp->sctp_flags = (SCTP_PCB_FLAGS_TCPTYPE |
		    SCTP_PCB_FLAGS_UNBOUND);
		sctp_feature_on(inp, SCTP_PCB_FLAGS_ZERO_COPY_ACTIVE);
#endif
	} else {
		



		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EOPNOTSUPP);
		so->so_pcb = NULL;
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		return (EOPNOTSUPP);
	}
	if (SCTP_BASE_SYSCTL(sctp_default_frag_interleave) == SCTP_FRAG_LEVEL_1) {
		sctp_feature_on(inp, SCTP_PCB_FLAGS_FRAG_INTERLEAVE);
		sctp_feature_off(inp, SCTP_PCB_FLAGS_INTERLEAVE_STRMS);
	} else if (SCTP_BASE_SYSCTL(sctp_default_frag_interleave) == SCTP_FRAG_LEVEL_2) {
		sctp_feature_on(inp, SCTP_PCB_FLAGS_FRAG_INTERLEAVE);
		sctp_feature_on(inp, SCTP_PCB_FLAGS_INTERLEAVE_STRMS);
	} else if (SCTP_BASE_SYSCTL(sctp_default_frag_interleave) == SCTP_FRAG_LEVEL_0) {
		sctp_feature_off(inp, SCTP_PCB_FLAGS_FRAG_INTERLEAVE);
		sctp_feature_off(inp, SCTP_PCB_FLAGS_INTERLEAVE_STRMS);
	}
	inp->sctp_tcbhash = SCTP_HASH_INIT(SCTP_BASE_SYSCTL(sctp_pcbtblsize),
					   &inp->sctp_hashmark);
	if (inp->sctp_tcbhash == NULL) {
		SCTP_PRINTF("Out of SCTP-INPCB->hashinit - no resources\n");
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOBUFS);
		so->so_pcb = NULL;
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		return (ENOBUFS);
	}
#ifdef SCTP_MVRF
	inp->vrf_size = SCTP_DEFAULT_VRF_SIZE;
	SCTP_MALLOC(inp->m_vrf_ids, uint32_t *,
		    (sizeof(uint32_t) * inp->vrf_size), SCTP_M_MVRF);
	if (inp->m_vrf_ids == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOBUFS);
		so->so_pcb = NULL;
		SCTP_HASH_FREE(inp->sctp_tcbhash, inp->sctp_hashmark);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		return (ENOBUFS);
	}
	inp->m_vrf_ids[0] = vrf_id;
	inp->num_vrfs = 1;
#endif
	inp->def_vrf_id = vrf_id;

#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
	inp->ip_inp.inp.inpcb_mtx = lck_mtx_alloc_init(SCTP_BASE_INFO(mtx_grp), SCTP_BASE_INFO(mtx_attr));
	if (inp->ip_inp.inp.inpcb_mtx == NULL) {
		SCTP_PRINTF("in_pcballoc: can't alloc mutex! so=%p\n", (void *)so);
#ifdef SCTP_MVRF
		SCTP_FREE(inp->m_vrf_ids, SCTP_M_MVRF);
#endif
		SCTP_HASH_FREE(inp->sctp_tcbhash, inp->sctp_hashmark);
		so->so_pcb = NULL;
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
		SCTP_UNLOCK_EXC(SCTP_BASE_INFO(ipi_ep_mtx));
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOMEM);
		return (ENOMEM);
	}
#else
	lck_mtx_init(&inp->ip_inp.inp.inpcb_mtx, SCTP_BASE_INFO(mtx_grp), SCTP_BASE_INFO(mtx_attr));
#endif
#endif
	SCTP_INP_INFO_WLOCK();
	SCTP_INP_LOCK_INIT(inp);
#if defined(__FreeBSD__)
	INP_LOCK_INIT(&inp->ip_inp.inp, "inp", "sctpinp");
#endif
	SCTP_INP_READ_INIT(inp);
	SCTP_ASOC_CREATE_LOCK_INIT(inp);
	
	SCTP_INP_WLOCK(inp);

	
	LIST_INSERT_HEAD(&SCTP_BASE_INFO(listhead), inp, sctp_list);
#if defined(__APPLE__)
	LIST_INSERT_HEAD(&SCTP_BASE_INFO(inplisthead), &inp->ip_inp.inp, inp_list);
#endif
	SCTP_INP_INFO_WUNLOCK();

	TAILQ_INIT(&inp->read_queue);
	LIST_INIT(&inp->sctp_addr_list);

	LIST_INIT(&inp->sctp_asoc_list);

#ifdef SCTP_TRACK_FREED_ASOCS
	
	LIST_INIT(&inp->sctp_asoc_free_list);
#endif
	
	SCTP_OS_TIMER_INIT(&inp->sctp_ep.signature_change.timer);
	inp->sctp_ep.signature_change.type = SCTP_TIMER_TYPE_NEWCOOKIE;

	
	m = &inp->sctp_ep;

	
	m->sctp_timeoutticks[SCTP_TIMER_SEND] = SEC_TO_TICKS(SCTP_SEND_SEC);	
	m->sctp_timeoutticks[SCTP_TIMER_INIT] = SEC_TO_TICKS(SCTP_INIT_SEC);	
	m->sctp_timeoutticks[SCTP_TIMER_RECV] = MSEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_delayed_sack_time_default));
	m->sctp_timeoutticks[SCTP_TIMER_HEARTBEAT] = MSEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_heartbeat_interval_default));
	m->sctp_timeoutticks[SCTP_TIMER_PMTU] = SEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_pmtu_raise_time_default));
	m->sctp_timeoutticks[SCTP_TIMER_MAXSHUTDOWN] = SEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_shutdown_guard_time_default));
	m->sctp_timeoutticks[SCTP_TIMER_SIGNATURE] = SEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_secret_lifetime_default));
	
	m->sctp_maxrto = SCTP_BASE_SYSCTL(sctp_rto_max_default);
	m->sctp_minrto = SCTP_BASE_SYSCTL(sctp_rto_min_default);
	m->initial_rto = SCTP_BASE_SYSCTL(sctp_rto_initial_default);
	m->initial_init_rto_max = SCTP_BASE_SYSCTL(sctp_init_rto_max_default);
	m->sctp_sack_freq = SCTP_BASE_SYSCTL(sctp_sack_freq_default);

	m->max_open_streams_intome = MAX_SCTP_STREAMS;

	m->max_init_times = SCTP_BASE_SYSCTL(sctp_init_rtx_max_default);
	m->max_send_times = SCTP_BASE_SYSCTL(sctp_assoc_rtx_max_default);
	m->def_net_failure = SCTP_BASE_SYSCTL(sctp_path_rtx_max_default);
	m->def_net_pf_threshold = SCTP_BASE_SYSCTL(sctp_path_pf_threshold);
	m->sctp_sws_sender = SCTP_SWS_SENDER_DEF;
	m->sctp_sws_receiver = SCTP_SWS_RECEIVER_DEF;
	m->max_burst = SCTP_BASE_SYSCTL(sctp_max_burst_default);
	m->fr_max_burst = SCTP_BASE_SYSCTL(sctp_fr_max_burst_default);

	m->sctp_default_cc_module = SCTP_BASE_SYSCTL(sctp_default_cc_module);
	m->sctp_default_ss_module = SCTP_BASE_SYSCTL(sctp_default_ss_module);
	
	m->pre_open_stream_count = SCTP_BASE_SYSCTL(sctp_nr_outgoing_streams_default);

	
	m->adaptation_layer_indicator = 0;
	m->adaptation_layer_indicator_provided = 0;

	
	m->random_counter = 1;
	m->store_at = SCTP_SIGNATURE_SIZE;
	SCTP_READ_RANDOM(m->random_numbers, sizeof(m->random_numbers));
	sctp_fill_random_store(m);

	
	m->size_of_a_cookie = (sizeof(struct sctp_init_msg) * 2) +
	    sizeof(struct sctp_state_cookie);
	m->size_of_a_cookie += SCTP_SIGNATURE_SIZE;

	
	(void)SCTP_GETTIME_TIMEVAL(&time);
	m->time_of_secret_change = time.tv_sec;

	for (i = 0; i < SCTP_NUMBER_OF_SECRETS; i++) {
		m->secret_key[0][i] = sctp_select_initial_TSN(m);
	}
	sctp_timer_start(SCTP_TIMER_TYPE_NEWCOOKIE, inp, NULL, NULL);

	
	m->def_cookie_life = MSEC_TO_TICKS(SCTP_BASE_SYSCTL(sctp_valid_cookie_life_default));
	


	m->local_hmacs = sctp_default_supported_hmaclist();
	m->local_auth_chunks = sctp_alloc_chunklist();
	m->default_dscp = 0;
#ifdef INET6
	m->default_flowlabel = 0;
#endif
	m->port = 0; 
	sctp_auth_set_default_chunks(m->local_auth_chunks);
	LIST_INIT(&m->shared_keys);
	
	null_key = sctp_alloc_sharedkey();
	sctp_insert_sharedkey(&m->shared_keys, null_key);
	SCTP_INP_WUNLOCK(inp);
#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, NULL, 12);
#endif
	return (error);
}


void
sctp_move_pcb_and_assoc(struct sctp_inpcb *old_inp, struct sctp_inpcb *new_inp,
    struct sctp_tcb *stcb)
{
	struct sctp_nets *net;
	uint16_t lport, rport;
	struct sctppcbhead *head;
	struct sctp_laddr *laddr, *oladdr;

	atomic_add_int(&stcb->asoc.refcnt, 1);
	SCTP_TCB_UNLOCK(stcb);
	SCTP_INP_INFO_WLOCK();
	SCTP_INP_WLOCK(old_inp);
	SCTP_INP_WLOCK(new_inp);
	SCTP_TCB_LOCK(stcb);
	atomic_subtract_int(&stcb->asoc.refcnt, 1);

	new_inp->sctp_ep.time_of_secret_change =
	    old_inp->sctp_ep.time_of_secret_change;
	memcpy(new_inp->sctp_ep.secret_key, old_inp->sctp_ep.secret_key,
	    sizeof(old_inp->sctp_ep.secret_key));
	new_inp->sctp_ep.current_secret_number =
	    old_inp->sctp_ep.current_secret_number;
	new_inp->sctp_ep.last_secret_number =
	    old_inp->sctp_ep.last_secret_number;
	new_inp->sctp_ep.size_of_a_cookie = old_inp->sctp_ep.size_of_a_cookie;

	
	stcb->sctp_socket = new_inp->sctp_socket;
	stcb->sctp_ep = new_inp;

	
	lport = new_inp->sctp_lport = old_inp->sctp_lport;
	rport = stcb->rport;
	
	LIST_REMOVE(stcb, sctp_tcbhash);
	LIST_REMOVE(stcb, sctp_tcblist);
	if (stcb->asoc.in_asocid_hash) {
		LIST_REMOVE(stcb, sctp_tcbasocidhash);
	}
	
	head = &SCTP_BASE_INFO(sctp_tcpephash)[SCTP_PCBHASH_ALLADDR((lport | rport), SCTP_BASE_INFO(hashtcpmark))];

	LIST_INSERT_HEAD(head, new_inp, sctp_hash);
	
	new_inp->sctp_flags &= ~SCTP_PCB_FLAGS_UNBOUND;

	
	LIST_INSERT_HEAD(&new_inp->sctp_asoc_list, stcb, sctp_tcblist);
	




	if (stcb->asoc.in_asocid_hash) {
		struct sctpasochead *lhd;
		lhd = &new_inp->sctp_asocidhash[SCTP_PCBHASH_ASOC(stcb->asoc.assoc_id,
			new_inp->hashasocidmark)];
		LIST_INSERT_HEAD(lhd, stcb, sctp_tcbasocidhash);
	}
	
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		sctp_timer_start(SCTP_TIMER_TYPE_PATHMTURAISE, new_inp,
		    stcb, net);
	}

	SCTP_INP_INFO_WUNLOCK();
	if (new_inp->sctp_tcbhash != NULL) {
		SCTP_HASH_FREE(new_inp->sctp_tcbhash, new_inp->sctp_hashmark);
		new_inp->sctp_tcbhash = NULL;
	}
	if ((new_inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0) {
		
		LIST_FOREACH(oladdr, &old_inp->sctp_addr_list, sctp_nxt_addr) {
			laddr = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_laddr), struct sctp_laddr);
			if (laddr == NULL) {
				




				SCTPDBG(SCTP_DEBUG_PCB1, "Association hosed in TCP model, out of laddr memory\n");
				continue;
			}
			SCTP_INCR_LADDR_COUNT();
			bzero(laddr, sizeof(*laddr));
			(void)SCTP_GETTIME_TIMEVAL(&laddr->start_time);
			laddr->ifa = oladdr->ifa;
			atomic_add_int(&laddr->ifa->refcount, 1);
			LIST_INSERT_HEAD(&new_inp->sctp_addr_list, laddr,
			    sctp_nxt_addr);
			new_inp->laddr_count++;
			if (oladdr == stcb->asoc.last_used_address) {
				stcb->asoc.last_used_address = laddr;
			}
		}
	}
	





	stcb->asoc.dack_timer.ep = (void *)new_inp;
	stcb->asoc.asconf_timer.ep = (void *)new_inp;
	stcb->asoc.strreset_timer.ep = (void *)new_inp;
	stcb->asoc.shut_guard_timer.ep = (void *)new_inp;
	stcb->asoc.autoclose_timer.ep = (void *)new_inp;
	stcb->asoc.delayed_event_timer.ep = (void *)new_inp;
	stcb->asoc.delete_prim_timer.ep = (void *)new_inp;
	
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		net->pmtu_timer.ep = (void *)new_inp;
		net->hb_timer.ep = (void *)new_inp;
		net->rxt_timer.ep = (void *)new_inp;
	}
	SCTP_INP_WUNLOCK(new_inp);
	SCTP_INP_WUNLOCK(old_inp);
}


#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Userspace__))




extern void in6_sin6_2_sin(struct sockaddr_in *, struct sockaddr_in6 *sin6);
#endif



int
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
sctp_inpcb_bind(struct socket *so, struct sockaddr *addr,
		struct sctp_ifa *sctp_ifap, struct thread *p)
#elif defined(__Windows__)
sctp_inpcb_bind(struct socket *so, struct sockaddr *addr,
		struct sctp_ifa *sctp_ifap, PKTHREAD p)
#else
sctp_inpcb_bind(struct socket *so, struct sockaddr *addr,
		struct sctp_ifa *sctp_ifap, struct proc *p)
#endif
{
	
	struct sctppcbhead *head;
	struct sctp_inpcb *inp, *inp_tmp;
	struct inpcb *ip_inp;
	int port_reuse_active = 0;
	int bindall;
#ifdef SCTP_MVRF
	int i;
#endif
	uint16_t lport;
	int error;
	uint32_t vrf_id;

	lport = 0;
	error = 0;
	bindall = 1;
	inp = (struct sctp_inpcb *)so->so_pcb;
	ip_inp = (struct inpcb *)so->so_pcb;
#ifdef SCTP_DEBUG
	if (addr) {
		SCTPDBG(SCTP_DEBUG_PCB1, "Bind called port: %d\n",
			ntohs(((struct sockaddr_in *)addr)->sin_port));
		SCTPDBG(SCTP_DEBUG_PCB1, "Addr: ");
		SCTPDBG_ADDR(SCTP_DEBUG_PCB1, addr);
	}
#endif
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) == 0) {
		
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		return (EINVAL);
	}
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
#ifdef INVARIANTS
	if (p == NULL)
		panic("null proc/thread");
#endif
#endif
	if (addr != NULL) {
		switch (addr->sa_family) {
#ifdef INET
		case AF_INET:
		{
			struct sockaddr_in *sin;

			
			if (SCTP_IPV6_V6ONLY(ip_inp)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
				return (EINVAL);
			}
#ifdef HAVE_SA_LEN
			if (addr->sa_len != sizeof(*sin)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
				return (EINVAL);
			}
#endif

			sin = (struct sockaddr_in *)addr;
			lport = sin->sin_port;
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
 			



 			if (p && (error = prison_local_ip4(p->td_ucred, &sin->sin_addr)) != 0) {
 				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, error);
 				return (error);
  			}
#endif
			if (sin->sin_addr.s_addr != INADDR_ANY) {
				bindall = 0;
			}
			break;
		}
#endif
#ifdef INET6
		case AF_INET6:
		{
			
			struct sockaddr_in6 *sin6;

			sin6 = (struct sockaddr_in6 *)addr;

#ifdef HAVE_SA_LEN
			if (addr->sa_len != sizeof(*sin6)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
				return (EINVAL);
			}
#endif
			lport = sin6->sin6_port;
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
  			



 			if (p && (error = prison_local_ip6(p->td_ucred, &sin6->sin6_addr,
 			    (SCTP_IPV6_V6ONLY(inp) != 0))) != 0) {
 				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, error);
 				return (error);
 			}
#endif
			if (!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
				bindall = 0;
#ifdef SCTP_EMBEDDED_V6_SCOPE
				
#if defined(SCTP_KAME)
				if (sa6_embedscope(sin6, MODULE_GLOBAL(ip6_use_defzone)) != 0) {
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
					return (EINVAL);
				}
#elif defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
				if (in6_embedscope(&sin6->sin6_addr, sin6, ip_inp, NULL) != 0) {
#else
				if (in6_embedscope(&sin6->sin6_addr, sin6, ip_inp, NULL, NULL) != 0) {
#endif
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
					return (EINVAL);
				}
#elif defined(__FreeBSD__)
				error = scope6_check_id(sin6, MODULE_GLOBAL(ip6_use_defzone));
				if (error != 0) {
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, error);
					return (error);
				}
#else
				if (in6_embedscope(&sin6->sin6_addr, sin6) != 0) {
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
					return (EINVAL);
				}
#endif
#endif 
			}
#ifndef SCOPEDROUTING
			
			sin6->sin6_scope_id = 0;
#endif 
			break;
		}
#endif
#if defined(__Userspace__)
		case AF_CONN:
		{
			struct sockaddr_conn *sconn;

#ifdef HAVE_SA_LEN
			if (addr->sa_len != sizeof(struct sockaddr_conn)) {
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
				return (EINVAL);
			}
#endif
			sconn = (struct sockaddr_conn *)addr;
			lport = sconn->sconn_port;
			if (sconn->sconn_addr != NULL) {
				bindall = 0;
			}
			break;
		}
#endif
		default:
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EAFNOSUPPORT);
			return (EAFNOSUPPORT);
		}
	}
	SCTP_INP_INFO_WLOCK();
	SCTP_INP_WLOCK(inp);
	
 	vrf_id = inp->def_vrf_id;

	
	SCTP_INP_INCR_REF(inp);
	if (lport) {
		



		
#if !defined(__Windows__)
		if (ntohs(lport) < IPPORT_RESERVED) {
			if (p && (error =
#ifdef __FreeBSD__
#if __FreeBSD_version > 602000
				  priv_check(p, PRIV_NETINET_RESERVEDPORT)
#elif __FreeBSD_version >= 500000
				  suser_cred(p->td_ucred, 0)
#else
				  suser(p)
#endif
#elif defined(__APPLE__)
				  suser(p->p_ucred, &p->p_acflag)
#elif defined(__Userspace__) /* must be true to use raw socket */
                                  1
#else
				  suser(p, 0)
#endif
				    )) {
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				SCTP_INP_INFO_WUNLOCK();
				return (error);
			}
#if defined(__Panda__)
			if (!SCTP_IS_PRIVILEDGED(so)) {
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				SCTP_INP_INFO_WUNLOCK();
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EACCES);
				return (EACCES);
			}
#endif
		}
#if !defined(__Panda__) && !defined(__Userspace__)
		if (p == NULL) {
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
			SCTP_INP_INFO_WUNLOCK();
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, error);
			return (error);
		}
#endif
#endif 
		SCTP_INP_WUNLOCK(inp);
		if (bindall) {
#ifdef SCTP_MVRF
			for (i = 0; i < inp->num_vrfs; i++) {
				vrf_id = inp->m_vrf_ids[i];
#else
				vrf_id = inp->def_vrf_id;
#endif
				inp_tmp = sctp_pcb_findep(addr, 0, 1, vrf_id);
				if (inp_tmp != NULL) {
					







					SCTP_INP_DECR_REF(inp_tmp);
					
					if ((sctp_is_feature_on(inp, SCTP_PCB_FLAGS_PORTREUSE)) &&
					    (sctp_is_feature_on(inp_tmp, SCTP_PCB_FLAGS_PORTREUSE))) {
						
						port_reuse_active = 1;
						goto continue_anyway;
					}
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_INFO_WUNLOCK();
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EADDRINUSE);
					return (EADDRINUSE);
				}
#ifdef SCTP_MVRF
			}
#endif
		} else {
			inp_tmp = sctp_pcb_findep(addr, 0, 1, vrf_id);
			if (inp_tmp != NULL) {
				






				SCTP_INP_DECR_REF(inp_tmp);
				
				if ((sctp_is_feature_on(inp, SCTP_PCB_FLAGS_PORTREUSE)) &&
				    (sctp_is_feature_on(inp_tmp, SCTP_PCB_FLAGS_PORTREUSE))) {
					
					port_reuse_active = 1;
					goto continue_anyway;
				}
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_INFO_WUNLOCK();
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EADDRINUSE);
				return (EADDRINUSE);
			}
		}
	continue_anyway:
		SCTP_INP_WLOCK(inp);
		if (bindall) {
			
			if ((port_reuse_active == 0) &&
			    (inp_tmp = sctp_isport_inuse(inp, lport, vrf_id))) {
				
				if ((sctp_is_feature_on(inp, SCTP_PCB_FLAGS_PORTREUSE)) &&
				    (sctp_is_feature_on(inp_tmp, SCTP_PCB_FLAGS_PORTREUSE))) {
					port_reuse_active = 1;
				} else {
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
					SCTP_INP_INFO_WUNLOCK();
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EADDRINUSE);
					return (EADDRINUSE);
				}
			}
		}
	} else {
		uint16_t first, last, candidate;
                uint16_t count;
		int done;

#if defined(__Windows__)
		first = 1;
		last = 0xffff;
#else
#if defined(__Userspace__)
                
#elif defined(__FreeBSD__) || defined(__APPLE__)
                if (ip_inp->inp_flags & INP_HIGHPORT) {
                        first = MODULE_GLOBAL(ipport_hifirstauto);
                        last  = MODULE_GLOBAL(ipport_hilastauto);
                } else if (ip_inp->inp_flags & INP_LOWPORT) {
			if (p && (error =
#ifdef __FreeBSD__
#if __FreeBSD_version > 602000
				  priv_check(p, PRIV_NETINET_RESERVEDPORT)
#elif __FreeBSD_version >= 500000
				  suser_cred(p->td_ucred, 0)
#else
				  suser(p)
#endif
#elif defined(__APPLE__)
				  suser(p->p_ucred, &p->p_acflag)
#else
				  suser(p, 0)
#endif
				    )) {
				SCTP_INP_DECR_REF(inp);
				SCTP_INP_WUNLOCK(inp);
				SCTP_INP_INFO_WUNLOCK();
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, error);
				return (error);
			}
                        first = MODULE_GLOBAL(ipport_lowfirstauto);
                        last  = MODULE_GLOBAL(ipport_lowlastauto);
                } else {
#endif
 			first = MODULE_GLOBAL(ipport_firstauto);
 			last = MODULE_GLOBAL(ipport_lastauto);
#if defined(__FreeBSD__) || defined(__APPLE__)
                }
#endif
#endif
		if (first > last) {
			uint16_t temp;

			temp = first;
			first = last;
			last = temp;
		}
		count = last - first + 1; 
		candidate = first + sctp_select_initial_TSN(&inp->sctp_ep) % (count);

		done = 0;
		while (!done) {
#ifdef SCTP_MVRF
			for (i = 0; i < inp->num_vrfs; i++) {
				if (sctp_isport_inuse(inp, htons(candidate), inp->m_vrf_ids[i]) != NULL) {
					break;
				}
			}
			if (i == inp->num_vrfs) {
				done = 1;
			}
#else
			if (sctp_isport_inuse(inp, htons(candidate), inp->def_vrf_id) == NULL) {
				done = 1;
			}
#endif
			if (!done) {
				if (--count == 0) {
					SCTP_INP_DECR_REF(inp);
					SCTP_INP_WUNLOCK(inp);
					SCTP_INP_INFO_WUNLOCK();
					SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EADDRINUSE);
					return (EADDRINUSE);
				}
				if (candidate == last)
					candidate = first;
				else
					candidate = candidate + 1;
			}
		}
		lport = htons(candidate);
	}
	SCTP_INP_DECR_REF(inp);
	if (inp->sctp_flags & (SCTP_PCB_FLAGS_SOCKET_GONE |
			       SCTP_PCB_FLAGS_SOCKET_ALLGONE)) {
		



		SCTP_INP_WUNLOCK(inp);
		SCTP_INP_INFO_WUNLOCK();
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		return (EINVAL);
	}
	
	if (bindall) {
		
		inp->sctp_flags |= SCTP_PCB_FLAGS_BOUNDALL;
		
		if (SCTP_BASE_SYSCTL(sctp_auto_asconf) == 0) {
			sctp_feature_off(inp, SCTP_PCB_FLAGS_DO_ASCONF);
			sctp_feature_off(inp, SCTP_PCB_FLAGS_AUTO_ASCONF);
		} else {
			sctp_feature_on(inp, SCTP_PCB_FLAGS_DO_ASCONF);
			sctp_feature_on(inp, SCTP_PCB_FLAGS_AUTO_ASCONF);
		}
		if (SCTP_BASE_SYSCTL(sctp_multiple_asconfs) == 0) {
			sctp_feature_off(inp, SCTP_PCB_FLAGS_MULTIPLE_ASCONFS);
		} else {
			sctp_feature_on(inp, SCTP_PCB_FLAGS_MULTIPLE_ASCONFS);
		}
		


		if (SCTP_BASE_SYSCTL(sctp_mobility_base) == 0) {
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_BASE);
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_PRIM_DELETED);
		} else {
			sctp_mobility_feature_on(inp, SCTP_MOBILITY_BASE);
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_PRIM_DELETED);
		}
		


		if (SCTP_BASE_SYSCTL(sctp_mobility_fasthandoff) == 0) {
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_FASTHANDOFF);
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_PRIM_DELETED);
		} else {
			sctp_mobility_feature_on(inp, SCTP_MOBILITY_FASTHANDOFF);
			sctp_mobility_feature_off(inp, SCTP_MOBILITY_PRIM_DELETED);
		}
	} else {
		









		struct sctp_ifa *ifa;
		struct sockaddr_storage store_sa;

		memset(&store_sa, 0, sizeof(store_sa));
		switch (addr->sa_family) {
#ifdef INET
		case AF_INET:
		{
			struct sockaddr_in *sin;

			sin = (struct sockaddr_in *)&store_sa;
			memcpy(sin, addr, sizeof(struct sockaddr_in));
			sin->sin_port = 0;
			break;
		}
#endif
#ifdef INET6
		case AF_INET6:
		{
			struct sockaddr_in6 *sin6;

			sin6 = (struct sockaddr_in6 *)&store_sa;
			memcpy(sin6, addr, sizeof(struct sockaddr_in6));
			sin6->sin6_port = 0;
			break;
		}
#endif
#if defined(__Userspace__)
		case AF_CONN:
		{
			struct sockaddr_conn *sconn;

			sconn = (struct sockaddr_conn *)&store_sa;
			memcpy(sconn, addr, sizeof(struct sockaddr_conn));
			sconn->sconn_port = 0;
			break;
		}
#endif
		default:
			break;
		}
		




		if (sctp_ifap != NULL) {
			ifa = sctp_ifap;
		} else {
			



			ifa = sctp_find_ifa_by_addr((struct sockaddr *)&store_sa,
						    vrf_id, SCTP_ADDR_NOT_LOCKED);
		}
		if (ifa == NULL) {
			
			SCTP_INP_WUNLOCK(inp);
			SCTP_INP_INFO_WUNLOCK();
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EADDRNOTAVAIL);
			return (EADDRNOTAVAIL);
		}
#ifdef INET6
		if (addr->sa_family == AF_INET6) {
			
			if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
				
				SCTP_INP_WUNLOCK(inp);
				SCTP_INP_INFO_WUNLOCK();
				SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
				return (EINVAL);
			}
		}
#endif
		
		inp->sctp_flags &= ~SCTP_PCB_FLAGS_BOUNDALL;
		
		sctp_feature_on(inp, SCTP_PCB_FLAGS_DO_ASCONF);
		
		sctp_feature_off(inp, SCTP_PCB_FLAGS_AUTO_ASCONF);

		
		error = sctp_insert_laddr(&inp->sctp_addr_list, ifa, 0);
		if (error != 0) {
			SCTP_INP_WUNLOCK(inp);
			SCTP_INP_INFO_WUNLOCK();
			return (error);
		}
		inp->laddr_count++;
	}
	
	if (port_reuse_active) {
		
		head = &SCTP_BASE_INFO(sctp_tcpephash)[SCTP_PCBHASH_ALLADDR(lport, SCTP_BASE_INFO(hashtcpmark))];
		inp->sctp_flags |= SCTP_PCB_FLAGS_IN_TCPPOOL;
	}  else {
		head = &SCTP_BASE_INFO(sctp_ephash)[SCTP_PCBHASH_ALLADDR(lport, SCTP_BASE_INFO(hashmark))];
	}
	
	LIST_INSERT_HEAD(head, inp, sctp_hash);
	SCTPDBG(SCTP_DEBUG_PCB1, "Main hash to bind at head:%p, bound port:%d - in tcp_pool=%d\n",
		(void *)head, ntohs(lport), port_reuse_active);
	
	inp->sctp_lport = lport;

	
	inp->sctp_flags &= ~SCTP_PCB_FLAGS_UNBOUND;
	SCTP_INP_WUNLOCK(inp);
	SCTP_INP_INFO_WUNLOCK();
	return (0);
}


static void
sctp_iterator_inp_being_freed(struct sctp_inpcb *inp)
{
	struct sctp_iterator *it, *nit;

	



	it = sctp_it_ctl.cur_it;
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
	if (it && (it->vn != curvnet)) {
		
		return;
	}
#endif
	if (it && (it->inp == inp)) {
		












		if (it->iterator_flags & SCTP_ITERATOR_DO_SINGLE_INP) {
			sctp_it_ctl.iterator_flags |= SCTP_ITERATOR_STOP_CUR_IT;
		} else {
			sctp_it_ctl.iterator_flags |= SCTP_ITERATOR_STOP_CUR_INP;
		}
	}
	


	SCTP_IPI_ITERATOR_WQ_LOCK();
	TAILQ_FOREACH_SAFE(it, &sctp_it_ctl.iteratorhead, sctp_nxt_itr, nit) {
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
		if (it->vn != curvnet) {
			continue;
		}
#endif
		if (it->inp == inp) {
			
			if (it->iterator_flags & SCTP_ITERATOR_DO_SINGLE_INP) {
				
				TAILQ_REMOVE(&sctp_it_ctl.iteratorhead,
				    it, sctp_nxt_itr);
				if (it->function_atend != NULL) {
					(*it->function_atend) (it->pointer, it->val);
				}
				SCTP_FREE(it, SCTP_M_ITER);
			} else {
				it->inp = LIST_NEXT(it->inp, sctp_list);
				if (it->inp) {
					SCTP_INP_INCR_REF(it->inp);
				}
			}
			
			SCTP_INP_DECR_REF(inp);
		}
	}
	SCTP_IPI_ITERATOR_WQ_UNLOCK();
}


void
sctp_inpcb_free(struct sctp_inpcb *inp, int immediate, int from)
{
	







	struct sctp_tcb *asoc, *nasoc;
	struct sctp_laddr *laddr, *nladdr;
	struct inpcb *ip_pcb;
	struct socket *so;
	int being_refed = 0;
	struct sctp_queued_to_read *sq, *nsq;
#if !defined(__Panda__) && !defined(__Userspace__)
#if !defined(__FreeBSD__) || __FreeBSD_version < 500000
	sctp_rtentry_t *rt;
#endif
#endif
	int cnt;
	sctp_sharedkey_t *shared_key, *nshared_key;


#if defined(__APPLE__)
	sctp_lock_assert(SCTP_INP_SO(inp));
#endif
#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, NULL, 0);
#endif
	SCTP_ITERATOR_LOCK();
	
	sctp_iterator_inp_being_freed(inp);
	SCTP_ITERATOR_UNLOCK();
	so = inp->sctp_socket;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
		
		SCTP_PRINTF("This conflict in free SHOULD not be happening! from %d, imm %d\n", from, immediate);
#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, NULL, 1);
#endif
		return;
	}
	SCTP_ASOC_CREATE_LOCK(inp);
	SCTP_INP_INFO_WLOCK();

	SCTP_INP_WLOCK(inp);
	if (from == SCTP_CALLED_AFTER_CMPSET_OFCLOSE) {
		inp->sctp_flags &= ~SCTP_PCB_FLAGS_CLOSE_IP;
		
		inp->sctp_flags |= SCTP_PCB_FLAGS_DONT_WAKE;
		inp->sctp_flags &= ~SCTP_PCB_FLAGS_WAKEINPUT;
		inp->sctp_flags &= ~SCTP_PCB_FLAGS_WAKEOUTPUT;

	}
	
	sctp_timer_stop(SCTP_TIMER_TYPE_NEWCOOKIE, inp, NULL, NULL,
			SCTP_FROM_SCTP_PCB+SCTP_LOC_1);

	if (inp->control) {
		sctp_m_freem(inp->control);
		inp->control = NULL;
	}
	if (inp->pkt) {
		sctp_m_freem(inp->pkt);
		inp->pkt = NULL;
	}
	ip_pcb = &inp->ip_inp.inp;	


	if (immediate == SCTP_FREE_SHOULD_USE_GRACEFUL_CLOSE) {
		int cnt_in_sd;

		cnt_in_sd = 0;
		LIST_FOREACH_SAFE(asoc, &inp->sctp_asoc_list, sctp_tcblist, nasoc) {
			SCTP_TCB_LOCK(asoc);
			if (asoc->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				
				cnt_in_sd++;
				if (asoc->asoc.state & SCTP_STATE_IN_ACCEPT_QUEUE) {
					




					asoc->asoc.state &= ~SCTP_STATE_IN_ACCEPT_QUEUE;
					sctp_timer_start(SCTP_TIMER_TYPE_ASOCKILL, inp, asoc, NULL);
				}
				SCTP_TCB_UNLOCK(asoc);
				continue;
			}
			if (((SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_COOKIE_WAIT) ||
			    (SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_COOKIE_ECHOED)) &&
			    (asoc->asoc.total_output_queue_size == 0)) {
				




				
				if (sctp_free_assoc(inp, asoc, SCTP_PCBFREE_NOFORCE,
						   SCTP_FROM_SCTP_PCB+SCTP_LOC_2) == 0) {
					cnt_in_sd++;
				}
				continue;
			}
			
			asoc->sctp_socket = NULL;
			asoc->asoc.state |= SCTP_STATE_CLOSED_SOCKET;
			if ((asoc->asoc.size_on_reasm_queue > 0) ||
			    (asoc->asoc.control_pdapi) ||
			    (asoc->asoc.size_on_all_streams > 0) ||
			    (so && (so->so_rcv.sb_cc > 0))) {
				
				struct mbuf *op_err;

				op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
							       0, M_NOWAIT, 1, MT_DATA);
				if (op_err) {
					
					struct sctp_paramhdr *ph;

					SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr);
					ph = mtod(op_err, struct sctp_paramhdr *);
					ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
					ph->param_length = htons(SCTP_BUF_LEN(op_err));
				}
				asoc->sctp_ep->last_abort_code = SCTP_FROM_SCTP_PCB+SCTP_LOC_3;
				sctp_send_abort_tcb(asoc, op_err, SCTP_SO_LOCKED);
				SCTP_STAT_INCR_COUNTER32(sctps_aborted);
				if ((SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_OPEN) ||
				    (SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
					SCTP_STAT_DECR_GAUGE32(sctps_currestab);
				}
				if (sctp_free_assoc(inp, asoc,
						    SCTP_PCBFREE_NOFORCE, SCTP_FROM_SCTP_PCB+SCTP_LOC_4) == 0) {
					cnt_in_sd++;
				}
				continue;
			} else if (TAILQ_EMPTY(&asoc->asoc.send_queue) &&
			           TAILQ_EMPTY(&asoc->asoc.sent_queue) &&
			           (asoc->asoc.stream_queue_cnt == 0)) {
				if (asoc->asoc.locked_on_sending) {
					goto abort_anyway;
				}
				if ((SCTP_GET_STATE(&asoc->asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
				    (SCTP_GET_STATE(&asoc->asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
					struct sctp_nets *netp;

					



					if ((SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_OPEN) ||
					    (SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
						SCTP_STAT_DECR_GAUGE32(sctps_currestab);
					}
					SCTP_SET_STATE(&asoc->asoc, SCTP_STATE_SHUTDOWN_SENT);
					SCTP_CLEAR_SUBSTATE(&asoc->asoc, SCTP_STATE_SHUTDOWN_PENDING);
					sctp_stop_timers_for_shutdown(asoc);
					if (asoc->asoc.alternate) {
						netp = asoc->asoc.alternate;
					} else {
						netp = asoc->asoc.primary_destination;
					}
					sctp_send_shutdown(asoc, netp);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN, asoc->sctp_ep, asoc,
					    netp);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, asoc->sctp_ep, asoc,
					    asoc->asoc.primary_destination);
					sctp_chunk_output(inp, asoc, SCTP_OUTPUT_FROM_SHUT_TMR, SCTP_SO_LOCKED);
				}
			} else {
				
				struct sctp_stream_queue_pending *sp;

				asoc->asoc.state |= SCTP_STATE_SHUTDOWN_PENDING;
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, asoc->sctp_ep, asoc,
						 asoc->asoc.primary_destination);
				if (asoc->asoc.locked_on_sending) {
					sp = TAILQ_LAST(&((asoc->asoc.locked_on_sending)->outqueue),
						sctp_streamhead);
					if (sp == NULL) {
						SCTP_PRINTF("Error, sp is NULL, locked on sending is %p strm:%d\n",
						       (void *)asoc->asoc.locked_on_sending,
						       asoc->asoc.locked_on_sending->stream_no);
					} else {
						if ((sp->length == 0) && (sp->msg_is_complete == 0))
							asoc->asoc.state |= SCTP_STATE_PARTIAL_MSG_LEFT;
					}
				}
				if (TAILQ_EMPTY(&asoc->asoc.send_queue) &&
				    TAILQ_EMPTY(&asoc->asoc.sent_queue) &&
				    (asoc->asoc.state & SCTP_STATE_PARTIAL_MSG_LEFT)) {
					struct mbuf *op_err;
				abort_anyway:
					op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
								       0, M_NOWAIT, 1, MT_DATA);
					if (op_err) {
						
						struct sctp_paramhdr *ph;

						SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr);
						ph = mtod(op_err, struct sctp_paramhdr *);
						ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
						ph->param_length = htons(SCTP_BUF_LEN(op_err));
					}
					asoc->sctp_ep->last_abort_code = SCTP_FROM_SCTP_PCB+SCTP_LOC_5;
					sctp_send_abort_tcb(asoc, op_err, SCTP_SO_LOCKED);
					SCTP_STAT_INCR_COUNTER32(sctps_aborted);
					if ((SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_OPEN) ||
					    (SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
						SCTP_STAT_DECR_GAUGE32(sctps_currestab);
					}
					if (sctp_free_assoc(inp, asoc,
							    SCTP_PCBFREE_NOFORCE,
							    SCTP_FROM_SCTP_PCB+SCTP_LOC_6) == 0) {
						cnt_in_sd++;
					}
					continue;
				} else {
					sctp_chunk_output(inp, asoc, SCTP_OUTPUT_FROM_CLOSING, SCTP_SO_LOCKED);
				}
			}
			cnt_in_sd++;
			SCTP_TCB_UNLOCK(asoc);
		}
		
		if (cnt_in_sd) {
#ifdef SCTP_LOG_CLOSING
			sctp_log_closing(inp, NULL, 2);
#endif
			inp->sctp_socket = NULL;
			SCTP_INP_WUNLOCK(inp);
			SCTP_ASOC_CREATE_UNLOCK(inp);
			SCTP_INP_INFO_WUNLOCK();
			return;
		}
	}
	inp->sctp_socket = NULL;
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) !=
	    SCTP_PCB_FLAGS_UNBOUND) {
		




		LIST_REMOVE(inp, sctp_hash);
		inp->sctp_flags |= SCTP_PCB_FLAGS_UNBOUND;
	}

	




	cnt = 0;
	LIST_FOREACH_SAFE(asoc, &inp->sctp_asoc_list, sctp_tcblist, nasoc) {
		SCTP_TCB_LOCK(asoc);
		if (asoc->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
			if (asoc->asoc.state & SCTP_STATE_IN_ACCEPT_QUEUE) {
				asoc->asoc.state &= ~SCTP_STATE_IN_ACCEPT_QUEUE;
				sctp_timer_start(SCTP_TIMER_TYPE_ASOCKILL, inp, asoc, NULL);
			}
		        cnt++;
			SCTP_TCB_UNLOCK(asoc);
			continue;
		}
		
		if ((SCTP_GET_STATE(&asoc->asoc) != SCTP_STATE_COOKIE_WAIT) &&
		    ((asoc->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) == 0)) {
			struct mbuf *op_err;

			op_err = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr),
						       0, M_NOWAIT, 1, MT_DATA);
			if (op_err) {
				
				struct sctp_paramhdr *ph;

				SCTP_BUF_LEN(op_err) = sizeof(struct sctp_paramhdr);
				ph = mtod(op_err, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
				ph->param_length = htons(SCTP_BUF_LEN(op_err));
			}
			asoc->sctp_ep->last_abort_code = SCTP_FROM_SCTP_PCB+SCTP_LOC_7;
			sctp_send_abort_tcb(asoc, op_err, SCTP_SO_LOCKED);
			SCTP_STAT_INCR_COUNTER32(sctps_aborted);
		} else if (asoc->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
			cnt++;
			SCTP_TCB_UNLOCK(asoc);
			continue;
		}
		if ((SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_OPEN) ||
		    (SCTP_GET_STATE(&asoc->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED)) {
			SCTP_STAT_DECR_GAUGE32(sctps_currestab);
		}
		if (sctp_free_assoc(inp, asoc, SCTP_PCBFREE_FORCE, SCTP_FROM_SCTP_PCB+SCTP_LOC_8) == 0) {
			cnt++;
		}
	}
	if (cnt) {
		
		(void)SCTP_OS_TIMER_STOP(&inp->sctp_ep.signature_change.timer);
#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, NULL, 3);
#endif
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		SCTP_INP_INFO_WUNLOCK();
		return;
	}
	if (SCTP_INP_LOCK_CONTENDED(inp))
		being_refed++;
	if (SCTP_INP_READ_CONTENDED(inp))
		being_refed++;
	if (SCTP_ASOC_CREATE_LOCK_CONTENDED(inp))
		being_refed++;

	if ((inp->refcount) ||
	    (being_refed) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_CLOSE_IP)) {
		(void)SCTP_OS_TIMER_STOP(&inp->sctp_ep.signature_change.timer);
#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, NULL, 4);
#endif
		sctp_timer_start(SCTP_TIMER_TYPE_INPKILL, inp, NULL, NULL);
		SCTP_INP_WUNLOCK(inp);
		SCTP_ASOC_CREATE_UNLOCK(inp);
		SCTP_INP_INFO_WUNLOCK();
		return;
	}
	inp->sctp_ep.signature_change.type = 0;
	inp->sctp_flags |= SCTP_PCB_FLAGS_SOCKET_ALLGONE;
	


	LIST_REMOVE(inp, sctp_list);
	SCTP_INP_WUNLOCK(inp);
	SCTP_ASOC_CREATE_UNLOCK(inp);
	SCTP_INP_INFO_WUNLOCK();
	






	if (from != SCTP_CALLED_FROM_INPKILL_TIMER) {
		(void)SCTP_OS_TIMER_STOP_DRAIN(&inp->sctp_ep.signature_change.timer);
	} else {
		
		(void)SCTP_OS_TIMER_STOP(&inp->sctp_ep.signature_change.timer);
	}

#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, NULL, 5);
#endif

#if !(defined(__Panda__) || defined(__Windows__) || defined(__Userspace__))
#if !defined(__FreeBSD__) || __FreeBSD_version < 500000
	rt = ip_pcb->inp_route.ro_rt;
#endif
#endif

#if defined(__Panda__)
	if (inp->pak_to_read) {
		(void)SCTP_OS_TIMER_STOP(&inp->sctp_ep.zero_copy_timer.timer);
		SCTP_RELEASE_PKT(inp->pak_to_read);
		inp->pak_to_read = NULL;
	}
	if (inp->pak_to_read_sendq) {
		(void)SCTP_OS_TIMER_STOP(&inp->sctp_ep.zero_copy_sendq_timer.timer);
		SCTP_RELEASE_PKT(inp->pak_to_read_sendq);
		inp->pak_to_read_sendq = NULL;
	}
#endif
	if ((inp->sctp_asocidhash) != NULL) {
		SCTP_HASH_FREE(inp->sctp_asocidhash, inp->hashasocidmark);
		inp->sctp_asocidhash = NULL;
	}
	
	TAILQ_FOREACH_SAFE(sq, &inp->read_queue, next, nsq) {
		
		if (sq->length)
			SCTP_STAT_INCR(sctps_left_abandon);

		TAILQ_REMOVE(&inp->read_queue, sq, next);
		sctp_free_remote_addr(sq->whoFrom);
		if (so)
			so->so_rcv.sb_cc -= sq->length;
		if (sq->data) {
			sctp_m_freem(sq->data);
			sq->data = NULL;
		}
		



		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_readq), sq);
		SCTP_DECR_READQ_COUNT();
	}
	
	




	if (so) {
#ifdef IPSEC
		ipsec_delete_pcbpolicy(ip_pcb);
#endif				

		
	}
#ifndef __Panda__
	if (ip_pcb->inp_options) {
		(void)sctp_m_free(ip_pcb->inp_options);
		ip_pcb->inp_options = 0;
	}
#endif

#if !(defined(__Panda__) || defined(__Windows__) || defined(__Userspace__))
#if !defined(__FreeBSD__) || __FreeBSD_version < 500000
	if (rt) {
		RTFREE(rt);
		ip_pcb->inp_route.ro_rt = 0;
	}
#endif
#if defined(__FreeBSD__) && __FreeBSD_version < 803000
#ifdef INET
	if (ip_pcb->inp_moptions) {
		inp_freemoptions(ip_pcb->inp_moptions);
		ip_pcb->inp_moptions = 0;
	}
#endif
#endif
#endif

#ifdef INET6
#if !(defined(__Panda__) || defined(__Windows__) || defined(__Userspace__))
#if defined(__FreeBSD__) || defined(__APPLE__)
	if (ip_pcb->inp_vflag & INP_IPV6) {
#else
	if (inp->inp_vflag & INP_IPV6) {
#endif
		struct in6pcb *in6p;

		in6p = (struct in6pcb *)inp;
		ip6_freepcbopts(in6p->in6p_outputopts);
	}
#endif
#endif				
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
	inp->inp_vflag = 0;
#else
	ip_pcb->inp_vflag = 0;
#endif
	
	if (inp->sctp_ep.local_auth_chunks != NULL)
		sctp_free_chunklist(inp->sctp_ep.local_auth_chunks);
	if (inp->sctp_ep.local_hmacs != NULL)
		sctp_free_hmaclist(inp->sctp_ep.local_hmacs);

	LIST_FOREACH_SAFE(shared_key, &inp->sctp_ep.shared_keys, next, nshared_key) {
		LIST_REMOVE(shared_key, next);
		sctp_free_sharedkey(shared_key);
		
	}

#if defined(__APPLE__)
	inp->ip_inp.inp.inp_state = INPCB_STATE_DEAD;
	if (in_pcb_checkstate(&inp->ip_inp.inp, WNT_STOPUSING, 1) != WNT_STOPUSING) {
#ifdef INVARIANTS
		panic("sctp_inpcb_free inp = %p couldn't set to STOPUSING\n", (void *)inp);
#else
		SCTP_PRINTF("sctp_inpcb_free inp = %p couldn't set to STOPUSING\n", (void *)inp);
#endif
	}
	inp->ip_inp.inp.inp_socket->so_flags |= SOF_PCBCLEARING;
#endif
	




	LIST_FOREACH_SAFE(laddr, &inp->sctp_addr_list, sctp_nxt_addr, nladdr) {
		sctp_remove_laddr(laddr);
	}

#ifdef SCTP_TRACK_FREED_ASOCS
	
	LIST_FOREACH_SAFE(asoc, &inp->sctp_asoc_free_list, sctp_tcblist, nasoc) {
		LIST_REMOVE(asoc, sctp_tcblist);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), asoc);
		SCTP_DECR_ASOC_COUNT();
	}
	
#endif
#ifdef SCTP_MVRF
	SCTP_FREE(inp->m_vrf_ids, SCTP_M_MVRF);
#endif
	
	if (inp->sctp_tcbhash != NULL) {
		SCTP_HASH_FREE(inp->sctp_tcbhash, inp->sctp_hashmark);
		inp->sctp_tcbhash = NULL;
	}
	
#if defined(__FreeBSD__)
	INP_LOCK_DESTROY(&inp->ip_inp.inp);
#endif
	SCTP_INP_LOCK_DESTROY(inp);
	SCTP_INP_READ_DESTROY(inp);
	SCTP_ASOC_CREATE_LOCK_DESTROY(inp);
#if !defined(__APPLE__)
	SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_ep), inp);
	SCTP_DECR_EP_COUNT();
#else
	
#endif
}


struct sctp_nets *
sctp_findnet(struct sctp_tcb *stcb, struct sockaddr *addr)
{
	struct sctp_nets *net;
	
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		if (sctp_cmpaddr(addr, (struct sockaddr *)&net->ro._l_addr))
			return (net);
	}
	return (NULL);
}


int
sctp_is_address_on_local_host(struct sockaddr *addr, uint32_t vrf_id)
{
#ifdef __Panda__
	return (0);
#else
	struct sctp_ifa *sctp_ifa;
	sctp_ifa = sctp_find_ifa_by_addr(addr, vrf_id, SCTP_ADDR_NOT_LOCKED);
	if (sctp_ifa) {
		return (1);
	} else {
		return (0);
	}
#endif
}






int
sctp_add_remote_addr(struct sctp_tcb *stcb, struct sockaddr *newaddr,
    struct sctp_nets **netp, int set_scope, int from)
{
	




	struct sctp_nets *net, *netfirst;
	int addr_inscope;

	SCTPDBG(SCTP_DEBUG_PCB1, "Adding an address (from:%d) to the peer: ",
		from);
	SCTPDBG_ADDR(SCTP_DEBUG_PCB1, newaddr);

	netfirst = sctp_findnet(stcb, newaddr);
	if (netfirst) {
		









		if (netfirst->dest_state & SCTP_ADDR_UNCONFIRMED) {
			netfirst->dest_state = (SCTP_ADDR_REACHABLE |
			    SCTP_ADDR_UNCONFIRMED);
		} else {
			netfirst->dest_state = SCTP_ADDR_REACHABLE;
		}

		return (0);
	}
	addr_inscope = 1;
	switch (newaddr->sa_family) {
#ifdef INET
	case AF_INET:
	{
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)newaddr;
		if (sin->sin_addr.s_addr == 0) {
			
			return (-1);
		}
		
		memset(&sin->sin_zero, 0, sizeof(sin->sin_zero));

		
#ifdef HAVE_SIN_LEN
		sin->sin_len = sizeof(struct sockaddr_in);
#endif
		if (set_scope) {
#ifdef SCTP_DONT_DO_PRIVADDR_SCOPE
			stcb->ipv4_local_scope = 1;
#else
			if (IN4_ISPRIVATE_ADDRESS(&sin->sin_addr)) {
				stcb->asoc.scope.ipv4_local_scope = 1;
			}
#endif				
		} else {
			
			if ((IN4_ISPRIVATE_ADDRESS(&sin->sin_addr)) &&
			    (stcb->asoc.scope.ipv4_local_scope == 0)) {
				addr_inscope = 0;
			}
		}
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)newaddr;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			
			return (-1);
		}
		
#ifdef HAVE_SIN6_LEN
		sin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
		if (set_scope) {
			if (sctp_is_address_on_local_host(newaddr, stcb->asoc.vrf_id)) {
				stcb->asoc.scope.loopback_scope = 1;
				stcb->asoc.scope.local_scope = 0;
				stcb->asoc.scope.ipv4_local_scope = 1;
				stcb->asoc.scope.site_scope = 1;
			} else if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) {
				







				stcb->asoc.scope.ipv4_local_scope = 1;
				stcb->asoc.scope.site_scope = 1;
			} else if (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr)) {
				



				stcb->asoc.scope.site_scope = 1;
			}
		} else {
			
			if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr) &&
			    (stcb->asoc.scope.loopback_scope == 0)) {
				addr_inscope = 0;
			} else if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) &&
			    (stcb->asoc.scope.local_scope == 0)) {
				addr_inscope = 0;
			} else if (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr) &&
			    (stcb->asoc.scope.site_scope == 0)) {
				addr_inscope = 0;
			}
		}
		break;
	}
#endif
#if defined(__Userspace__)
	case AF_CONN:
	{
		struct sockaddr_conn *sconn;

		sconn = (struct sockaddr_conn *)newaddr;
		if (sconn->sconn_addr == NULL) {
			
			return (-1);
		}
#ifdef HAVE_SCONN_LEN
		sconn->sconn_len = sizeof(struct sockaddr_conn);
#endif
		break;
	}
#endif
	default:
		
		return (-1);
	}
	net = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_net), struct sctp_nets);
	if (net == NULL) {
		return (-1);
	}
	SCTP_INCR_RADDR_COUNT();
	bzero(net, sizeof(struct sctp_nets));
	(void)SCTP_GETTIME_TIMEVAL(&net->start_time);
#ifdef HAVE_SA_LEN
	memcpy(&net->ro._l_addr, newaddr, newaddr->sa_len);
#endif
	switch (newaddr->sa_family) {
#ifdef INET
	case AF_INET:
#ifndef HAVE_SA_LEN
		memcpy(&net->ro._l_addr, newaddr, sizeof(struct sockaddr_in));
#endif
		((struct sockaddr_in *)&net->ro._l_addr)->sin_port = stcb->rport;
		break;
#endif
#ifdef INET6
	case AF_INET6:
#ifndef HAVE_SA_LEN
		memcpy(&net->ro._l_addr, newaddr, sizeof(struct sockaddr_in6));
#endif
		((struct sockaddr_in6 *)&net->ro._l_addr)->sin6_port = stcb->rport;
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
#ifndef HAVE_SA_LEN
		memcpy(&net->ro._l_addr, newaddr, sizeof(struct sockaddr_conn));
#endif
		((struct sockaddr_conn *)&net->ro._l_addr)->sconn_port = stcb->rport;
		break;
#endif
	default:
		break;
	}
	net->addr_is_local = sctp_is_address_on_local_host(newaddr, stcb->asoc.vrf_id);
	if (net->addr_is_local && ((set_scope || (from == SCTP_ADDR_IS_CONFIRMED)))) {
		stcb->asoc.scope.loopback_scope = 1;
		stcb->asoc.scope.ipv4_local_scope = 1;
		stcb->asoc.scope.local_scope = 0;
		stcb->asoc.scope.site_scope = 1;
		addr_inscope = 1;
	}
	net->failure_threshold = stcb->asoc.def_net_failure;
	net->pf_threshold = stcb->asoc.def_net_pf_threshold;
	if (addr_inscope == 0) {
		net->dest_state = (SCTP_ADDR_REACHABLE |
		    SCTP_ADDR_OUT_OF_SCOPE);
	} else {
		if (from == SCTP_ADDR_IS_CONFIRMED)
			
			net->dest_state = SCTP_ADDR_REACHABLE;
		else
			net->dest_state = SCTP_ADDR_REACHABLE |
			    SCTP_ADDR_UNCONFIRMED;
	}
	


	net->rto_needed = 1;
 	net->RTO = 0;
	net->RTO_measured = 0;
	stcb->asoc.numnets++;
	net->ref_count = 1;
	net->cwr_window_tsn = net->last_cwr_tsn = stcb->asoc.sending_seq - 1;
	net->port = stcb->asoc.port;
	net->dscp = stcb->asoc.default_dscp;
#ifdef INET6
	net->flowlabel = stcb->asoc.default_flowlabel;
#endif
	if (sctp_stcb_is_feature_on(stcb->sctp_ep, stcb, SCTP_PCB_FLAGS_DONOT_HEARTBEAT)) {
		net->dest_state |= SCTP_ADDR_NOHB;
	} else {
		net->dest_state &= ~SCTP_ADDR_NOHB;
	}
	if (sctp_stcb_is_feature_on(stcb->sctp_ep, stcb, SCTP_PCB_FLAGS_DO_NOT_PMTUD)) {
		net->dest_state |= SCTP_ADDR_NO_PMTUD;
	} else {
		net->dest_state &= ~SCTP_ADDR_NO_PMTUD;
	}
	net->heart_beat_delay = stcb->asoc.heart_beat_delay;
	
	SCTP_OS_TIMER_INIT(&net->rxt_timer.timer);
	SCTP_OS_TIMER_INIT(&net->pmtu_timer.timer);
	SCTP_OS_TIMER_INIT(&net->hb_timer.timer);

	
#ifdef INET6
#ifdef SCTP_EMBEDDED_V6_SCOPE
	
	if (newaddr->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
		(void)in6_embedscope(&sin6->sin6_addr, sin6, &stcb->sctp_ep->ip_inp.inp, NULL);
#else
		(void)in6_embedscope(&sin6->sin6_addr, sin6, &stcb->sctp_ep->ip_inp.inp, NULL, NULL);
#endif
#elif defined(SCTP_KAME)
		(void)sa6_embedscope(sin6, MODULE_GLOBAL(ip6_use_defzone));
#else
		(void)in6_embedscope(&sin6->sin6_addr, sin6);
#endif
#ifndef SCOPEDROUTING
		sin6->sin6_scope_id = 0;
#endif
	}
#endif 
#endif
	SCTP_RTALLOC((sctp_route_t *)&net->ro, stcb->asoc.vrf_id);

	if (SCTP_ROUTE_HAS_VALID_IFN(&net->ro)) {
		
		net->ro._s_addr = sctp_source_address_selection(stcb->sctp_ep,
								stcb,
								(sctp_route_t *)&net->ro,
								net,
								0,
								stcb->asoc.vrf_id);
		
		if (net->ro._s_addr && net->ro._s_addr->ifn_p) {
			net->mtu = SCTP_GATHER_MTU_FROM_INTFC(net->ro._s_addr->ifn_p);
		}
		if (net->mtu > 0) {
			uint32_t rmtu;

			rmtu = SCTP_GATHER_MTU_FROM_ROUTE(net->ro._s_addr, &net->ro._l_addr.sa, net->ro.ro_rt);
			if (rmtu == 0) {
				
				SCTP_SET_MTU_OF_ROUTE(&net->ro._l_addr.sa,
						      net->ro.ro_rt, net->mtu);
			} else {
				



 				net->mtu = rmtu;
			}
	        }
	}
	if (net->mtu == 0) {
		switch (newaddr->sa_family) {
#ifdef INET
		case AF_INET:
			net->mtu = SCTP_DEFAULT_MTU;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			net->mtu = 1280;
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			net->mtu = 1280;
			break;
#endif
		default:
			break;
		}
	}
	if (net->port) {
		net->mtu -= (uint32_t)sizeof(struct udphdr);
	}
	if (from == SCTP_ALLOC_ASOC) {
		stcb->asoc.smallest_mtu = net->mtu;
	}
	if (stcb->asoc.smallest_mtu > net->mtu) {
		stcb->asoc.smallest_mtu = net->mtu;
	}
#ifdef INET6
#ifdef SCTP_EMBEDDED_V6_SCOPE
	if (newaddr->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
#ifdef SCTP_KAME
		(void)sa6_recoverscope(sin6);
#else
		(void)in6_recoverscope(sin6, &sin6->sin6_addr, NULL);
#endif 
	}
#endif 
#endif

	
	if (stcb->asoc.cc_functions.sctp_set_initial_cc_param != NULL)
		(*stcb->asoc.cc_functions.sctp_set_initial_cc_param)(stcb, net);

	



	net->find_pseudo_cumack = 1;
	net->find_rtx_pseudo_cumack = 1;
	net->src_addr_selected = 0;
#if defined(__FreeBSD__)
	
	net->flowid = stcb->asoc.my_vtag ^
	              ntohs(stcb->rport) ^
	              ntohs(stcb->sctp_ep->sctp_lport);
#ifdef INVARIANTS
	net->flowidset = 1;
#endif
#endif
	if (netp) {
		*netp = net;
	}
	netfirst = TAILQ_FIRST(&stcb->asoc.nets);
	if (net->ro.ro_rt == NULL) {
		
		TAILQ_INSERT_TAIL(&stcb->asoc.nets, net, sctp_next);
	} else if (netfirst == NULL) {
		
		TAILQ_INSERT_HEAD(&stcb->asoc.nets, net, sctp_next);
	} else if (netfirst->ro.ro_rt == NULL) {
		



		TAILQ_INSERT_HEAD(&stcb->asoc.nets, net, sctp_next);
#ifndef __Panda__
	} else if (net->ro.ro_rt->rt_ifp != netfirst->ro.ro_rt->rt_ifp) {
		



		TAILQ_INSERT_HEAD(&stcb->asoc.nets, net, sctp_next);
#endif
	} else {
		





		struct sctp_nets *netlook;

		do {
			netlook = TAILQ_NEXT(netfirst, sctp_next);
			if (netlook == NULL) {
				
				TAILQ_INSERT_TAIL(&stcb->asoc.nets, net, sctp_next);
				break;
			} else if (netlook->ro.ro_rt == NULL) {
				
				TAILQ_INSERT_BEFORE(netfirst, net, sctp_next);
				break;
			}
#ifndef __Panda__
			else if (netlook->ro.ro_rt->rt_ifp != net->ro.ro_rt->rt_ifp)
#else
			else
#endif
			{
				TAILQ_INSERT_AFTER(&stcb->asoc.nets, netlook,
						   net, sctp_next);
				break;
			}
#ifndef __Panda__
			
			netfirst = netlook;
#endif
		} while (netlook != NULL);
	}

	
	if (stcb->asoc.primary_destination == 0) {
		stcb->asoc.primary_destination = net;
	} else if ((stcb->asoc.primary_destination->ro.ro_rt == NULL) &&
		    (net->ro.ro_rt) &&
	    ((net->dest_state & SCTP_ADDR_UNCONFIRMED) == 0)) {
		
		stcb->asoc.primary_destination = net;
	}
	
	net = TAILQ_FIRST(&stcb->asoc.nets);
	if ((net != stcb->asoc.primary_destination) &&
	    (stcb->asoc.primary_destination)) {
		




		TAILQ_REMOVE(&stcb->asoc.nets,
			     stcb->asoc.primary_destination, sctp_next);
		TAILQ_INSERT_HEAD(&stcb->asoc.nets,
				  stcb->asoc.primary_destination, sctp_next);
	}
	return (0);
}


static uint32_t
sctp_aloc_a_assoc_id(struct sctp_inpcb *inp, struct sctp_tcb *stcb)
{
	uint32_t id;
	struct sctpasochead *head;
	struct sctp_tcb *lstcb;

	SCTP_INP_WLOCK(inp);
 try_again:
	if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
		
		SCTP_INP_WUNLOCK(inp);
		return (0);
	}
	



	if (inp->sctp_associd_counter <= SCTP_ALL_ASSOC) {
		inp->sctp_associd_counter = SCTP_ALL_ASSOC + 1;
	}
	id = inp->sctp_associd_counter;
	inp->sctp_associd_counter++;
	lstcb = sctp_findasoc_ep_asocid_locked(inp, (sctp_assoc_t)id, 0);
	if (lstcb) {
		goto try_again;
	}
	head = &inp->sctp_asocidhash[SCTP_PCBHASH_ASOC(id, inp->hashasocidmark)];
	LIST_INSERT_HEAD(head, stcb, sctp_tcbasocidhash);
	stcb->asoc.in_asocid_hash = 1;
	SCTP_INP_WUNLOCK(inp);
	return id;
}






struct sctp_tcb *
sctp_aloc_assoc(struct sctp_inpcb *inp, struct sockaddr *firstaddr,
		int *error, uint32_t override_tag, uint32_t vrf_id,
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
		struct thread *p
#elif defined(__Windows__)
		PKTHREAD p
#else
#if defined(__Userspace__)
                
#endif
		struct proc *p
#endif
)
{
	

	struct sctp_tcb *stcb;
	struct sctp_association *asoc;
	struct sctpasochead *head;
	uint16_t rport;
	int err;

	




	if (SCTP_BASE_INFO(ipi_count_asoc) >= SCTP_MAX_NUM_OF_ASOC) {
		
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOBUFS);
		*error = ENOBUFS;
		return (NULL);
	}
	if (firstaddr == NULL) {
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		*error = EINVAL;
		return (NULL);
	}
	SCTP_INP_RLOCK(inp);
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) &&
	    ((sctp_is_feature_off(inp, SCTP_PCB_FLAGS_PORTREUSE)) ||
	     (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED))) {
		





		SCTP_INP_RUNLOCK(inp);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		*error = EINVAL;
		return (NULL);
	}
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE)) {
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_WAS_CONNECTED) ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_WAS_ABORTED)) {
			SCTP_INP_RUNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
			*error = EINVAL;
			return (NULL);
		}
	}
	SCTPDBG(SCTP_DEBUG_PCB3, "Allocate an association for peer:");
#ifdef SCTP_DEBUG
	if (firstaddr) {
		SCTPDBG_ADDR(SCTP_DEBUG_PCB3, firstaddr);
		switch (firstaddr->sa_family) {
#ifdef INET
		case AF_INET:
			SCTPDBG(SCTP_DEBUG_PCB3, "Port:%d\n",
			        ntohs(((struct sockaddr_in *)firstaddr)->sin_port));
			break;
#endif
#ifdef INET6
		case AF_INET6:
			SCTPDBG(SCTP_DEBUG_PCB3, "Port:%d\n",
			        ntohs(((struct sockaddr_in6 *)firstaddr)->sin6_port));
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			SCTPDBG(SCTP_DEBUG_PCB3, "Port:%d\n",
			        ntohs(((struct sockaddr_conn *)firstaddr)->sconn_port));
			break;
#endif
		default:
			break;
		}
	} else {
		SCTPDBG(SCTP_DEBUG_PCB3,"None\n");
	}
#endif				
	switch (firstaddr->sa_family) {
#ifdef INET
	case AF_INET:
	{
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)firstaddr;
		if ((ntohs(sin->sin_port) == 0) ||
		    (sin->sin_addr.s_addr == INADDR_ANY) ||
		    (sin->sin_addr.s_addr == INADDR_BROADCAST) ||
		    IN_MULTICAST(ntohl(sin->sin_addr.s_addr))) {
			
			SCTP_INP_RUNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
			*error = EINVAL;
			return (NULL);
		}
		rport = sin->sin_port;
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)firstaddr;
		if ((ntohs(sin6->sin6_port) == 0) ||
		    IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) ||
		    IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
			
			SCTP_INP_RUNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
			*error = EINVAL;
			return (NULL);
		}
		rport = sin6->sin6_port;
		break;
	}
#endif
#if defined(__Userspace__)
	case AF_CONN:
	{
		struct sockaddr_conn *sconn;

		sconn = (struct sockaddr_conn *)firstaddr;
		if ((ntohs(sconn->sconn_port) == 0) ||
		    (sconn->sconn_addr == NULL)) {
			
			SCTP_INP_RUNLOCK(inp);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
			*error = EINVAL;
			return (NULL);
		}
		rport = sconn->sconn_port;
		break;
	}
#endif
	default:
		
		SCTP_INP_RUNLOCK(inp);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		*error = EINVAL;
		return (NULL);
	}
	SCTP_INP_RUNLOCK(inp);
	if (inp->sctp_flags & SCTP_PCB_FLAGS_UNBOUND) {
		



		if ((err = sctp_inpcb_bind(inp->sctp_socket,
		    (struct sockaddr *)NULL,
		    (struct sctp_ifa *)NULL,
#ifndef __Panda__
					   p
#else
					   (struct proc *)NULL
#endif
		    ))) {
			
			*error = err;
			return (NULL);
		}
	}
	stcb = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_asoc), struct sctp_tcb);
	if (stcb == NULL) {
		
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOMEM);
		*error = ENOMEM;
		return (NULL);
	}
	SCTP_INCR_ASOC_COUNT();

	bzero(stcb, sizeof(*stcb));
	asoc = &stcb->asoc;

	asoc->assoc_id = sctp_aloc_a_assoc_id(inp, stcb);
	SCTP_TCB_LOCK_INIT(stcb);
	SCTP_TCB_SEND_LOCK_INIT(stcb);
	stcb->rport = rport;
	
	stcb->sctp_ep = inp;
	stcb->sctp_socket = inp->sctp_socket;
	if ((err = sctp_init_asoc(inp, stcb, override_tag, vrf_id))) {
		
		SCTP_TCB_LOCK_DESTROY(stcb);
		SCTP_TCB_SEND_LOCK_DESTROY(stcb);
		LIST_REMOVE(stcb, sctp_tcbasocidhash);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), stcb);
		SCTP_DECR_ASOC_COUNT();
		*error = err;
		return (NULL);
	}
	
	SCTP_INP_INFO_WLOCK();
	SCTP_INP_WLOCK(inp);
	if (inp->sctp_flags & (SCTP_PCB_FLAGS_SOCKET_GONE | SCTP_PCB_FLAGS_SOCKET_ALLGONE)) {
		
		SCTP_TCB_LOCK_DESTROY(stcb);
		SCTP_TCB_SEND_LOCK_DESTROY(stcb);
		LIST_REMOVE(stcb, sctp_tcbasocidhash);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), stcb);
		SCTP_INP_WUNLOCK(inp);
		SCTP_INP_INFO_WUNLOCK();
		SCTP_DECR_ASOC_COUNT();
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		*error = EINVAL;
		return (NULL);
	}
	SCTP_TCB_LOCK(stcb);

	
	head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(stcb->asoc.my_vtag, SCTP_BASE_INFO(hashasocmark))];
	
	LIST_INSERT_HEAD(head, stcb, sctp_asocs);
	SCTP_INP_INFO_WUNLOCK();

	if ((err = sctp_add_remote_addr(stcb, firstaddr, NULL, SCTP_DO_SETSCOPE, SCTP_ALLOC_ASOC))) {
		
		if (asoc->strmout) {
			SCTP_FREE(asoc->strmout, SCTP_M_STRMO);
			asoc->strmout = NULL;
		}
		if (asoc->mapping_array) {
			SCTP_FREE(asoc->mapping_array, SCTP_M_MAP);
			asoc->mapping_array = NULL;
		}
		if (asoc->nr_mapping_array) {
			SCTP_FREE(asoc->nr_mapping_array, SCTP_M_MAP);
			asoc->nr_mapping_array = NULL;
		}
		SCTP_DECR_ASOC_COUNT();
		SCTP_TCB_LOCK_DESTROY(stcb);
		SCTP_TCB_SEND_LOCK_DESTROY(stcb);
		LIST_REMOVE(stcb, sctp_tcbasocidhash);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), stcb);
		SCTP_INP_WUNLOCK(inp);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOBUFS);
		*error = ENOBUFS;
		return (NULL);
	}
	
	SCTP_OS_TIMER_INIT(&asoc->dack_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->strreset_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->asconf_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->shut_guard_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->autoclose_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->delayed_event_timer.timer);
	SCTP_OS_TIMER_INIT(&asoc->delete_prim_timer.timer);

	LIST_INSERT_HEAD(&inp->sctp_asoc_list, stcb, sctp_tcblist);
	
	if (inp->sctp_tcbhash != NULL) {
		head = &inp->sctp_tcbhash[SCTP_PCBHASH_ALLADDR(stcb->rport,
		    inp->sctp_hashmark)];
		LIST_INSERT_HEAD(head, stcb, sctp_tcbhash);
	}
	SCTP_INP_WUNLOCK(inp);
	SCTPDBG(SCTP_DEBUG_PCB1, "Association %p now allocated\n", (void *)stcb);
	return (stcb);
}


void
sctp_remove_net(struct sctp_tcb *stcb, struct sctp_nets *net)
{
	struct sctp_association *asoc;

	asoc = &stcb->asoc;
	asoc->numnets--;
	TAILQ_REMOVE(&asoc->nets, net, sctp_next);
	if (net == asoc->primary_destination) {
		
		struct sctp_nets *lnet;

		lnet = TAILQ_FIRST(&asoc->nets);
		




		if (sctp_is_mobility_feature_on(stcb->sctp_ep,
						SCTP_MOBILITY_BASE) ||
		    sctp_is_mobility_feature_on(stcb->sctp_ep,
			    			SCTP_MOBILITY_FASTHANDOFF)) {
			SCTPDBG(SCTP_DEBUG_ASCONF1, "remove_net: primary dst is deleting\n");
			if (asoc->deleted_primary != NULL) {
				SCTPDBG(SCTP_DEBUG_ASCONF1, "remove_net: deleted primary may be already stored\n");
				goto out;
			}
			asoc->deleted_primary = net;
			atomic_add_int(&net->ref_count, 1);
			memset(&net->lastsa, 0, sizeof(net->lastsa));
			memset(&net->lastsv, 0, sizeof(net->lastsv));
			sctp_mobility_feature_on(stcb->sctp_ep,
						 SCTP_MOBILITY_PRIM_DELETED);
			sctp_timer_start(SCTP_TIMER_TYPE_PRIM_DELETED,
					 stcb->sctp_ep, stcb, NULL);
		}
out:
		
		asoc->primary_destination = sctp_find_alternate_net(stcb, lnet, 0);
	}
	if (net == asoc->last_data_chunk_from) {
		
		asoc->last_data_chunk_from = TAILQ_FIRST(&asoc->nets);
	}
	if (net == asoc->last_control_chunk_from) {
		
		asoc->last_control_chunk_from = NULL;
	}
	if (net == stcb->asoc.alternate) {
		sctp_free_remote_addr(stcb->asoc.alternate);
		stcb->asoc.alternate = NULL;
	}
	sctp_free_remote_addr(net);
}





int
sctp_del_remote_addr(struct sctp_tcb *stcb, struct sockaddr *remaddr)
{
	






	struct sctp_association *asoc;
	struct sctp_nets *net, *nnet;

	asoc = &stcb->asoc;

	
	TAILQ_FOREACH_SAFE(net, &asoc->nets, sctp_next, nnet) {
		if (net->ro._l_addr.sa.sa_family != remaddr->sa_family) {
			continue;
		}
		if (sctp_cmpaddr((struct sockaddr *)&net->ro._l_addr,
		    remaddr)) {
			
			if (asoc->numnets < 2) {
				
				return (-1);
			} else {
				sctp_remove_net(stcb, net);
				return (0);
			}
		}
	}
	
	return (-2);
}

void
sctp_delete_from_timewait(uint32_t tag, uint16_t lport, uint16_t rport)
{
	struct sctpvtaghead *chain;
	struct sctp_tagblock *twait_block;
	int found = 0;
	int i;

	chain = &SCTP_BASE_INFO(vtag_timewait)[(tag % SCTP_STACK_VTAG_HASH_SIZE)];
	if (!LIST_EMPTY(chain)) {
		LIST_FOREACH(twait_block, chain, sctp_nxt_tagblock) {
			for (i = 0; i < SCTP_NUMBER_IN_VTAG_BLOCK; i++) {
			  if ((twait_block->vtag_block[i].v_tag == tag) &&
			      (twait_block->vtag_block[i].lport == lport) &&
			      (twait_block->vtag_block[i].rport == rport)) {
					twait_block->vtag_block[i].tv_sec_at_expire = 0;
					twait_block->vtag_block[i].v_tag = 0;
					twait_block->vtag_block[i].lport = 0;
					twait_block->vtag_block[i].rport = 0;
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}
}

int
sctp_is_in_timewait(uint32_t tag, uint16_t lport, uint16_t rport)
{
	struct sctpvtaghead *chain;
	struct sctp_tagblock *twait_block;
	int found = 0;
	int i;

	SCTP_INP_INFO_WLOCK();
	chain = &SCTP_BASE_INFO(vtag_timewait)[(tag % SCTP_STACK_VTAG_HASH_SIZE)];
	if (!LIST_EMPTY(chain)) {
		LIST_FOREACH(twait_block, chain, sctp_nxt_tagblock) {
			for (i = 0; i < SCTP_NUMBER_IN_VTAG_BLOCK; i++) {
			        if ((twait_block->vtag_block[i].v_tag == tag)  &&
				    (twait_block->vtag_block[i].lport == lport)  &&
				    (twait_block->vtag_block[i].rport == rport)) {
					found = 1;
					break;
				}
			}
			if (found)
				break;
		}
	}
	SCTP_INP_INFO_WUNLOCK();
	return (found);
}


void
sctp_add_vtag_to_timewait(uint32_t tag, uint32_t time, uint16_t lport, uint16_t rport)
{
	struct sctpvtaghead *chain;
	struct sctp_tagblock *twait_block;
	struct timeval now;
	int set, i;

	if (time == 0) {
		
		return;
	}
	(void)SCTP_GETTIME_TIMEVAL(&now);
	chain = &SCTP_BASE_INFO(vtag_timewait)[(tag % SCTP_STACK_VTAG_HASH_SIZE)];
	set = 0;
	if (!LIST_EMPTY(chain)) {
		
		LIST_FOREACH(twait_block, chain, sctp_nxt_tagblock) {
			for (i = 0; i < SCTP_NUMBER_IN_VTAG_BLOCK; i++) {
				if ((twait_block->vtag_block[i].v_tag == 0) &&
				    !set) {
					twait_block->vtag_block[i].tv_sec_at_expire =
						now.tv_sec + time;
					twait_block->vtag_block[i].v_tag = tag;
					twait_block->vtag_block[i].lport = lport;
					twait_block->vtag_block[i].rport = rport;
					set = 1;
				} else if ((twait_block->vtag_block[i].v_tag) &&
					    ((long)twait_block->vtag_block[i].tv_sec_at_expire < now.tv_sec)) {
					
					twait_block->vtag_block[i].tv_sec_at_expire = 0;
					twait_block->vtag_block[i].v_tag = 0;
					twait_block->vtag_block[i].lport = 0;
					twait_block->vtag_block[i].rport = 0;
					if (set == 0) {
						
						twait_block->vtag_block[i].tv_sec_at_expire = now.tv_sec + time;
						twait_block->vtag_block[i].v_tag = tag;
						twait_block->vtag_block[i].lport = lport;
						twait_block->vtag_block[i].rport = rport;
						set = 1;
					}
				}
			}
			if (set) {
				



				break;
			}
		}
	}
	
	if (!set) {
		SCTP_MALLOC(twait_block, struct sctp_tagblock *,
		    sizeof(struct sctp_tagblock), SCTP_M_TIMW);
		if (twait_block == NULL) {
#ifdef INVARIANTS
			panic("Can not alloc tagblock");
#endif
			return;
		}
		memset(twait_block, 0, sizeof(struct sctp_tagblock));
		LIST_INSERT_HEAD(chain, twait_block, sctp_nxt_tagblock);
		twait_block->vtag_block[0].tv_sec_at_expire = now.tv_sec + time;
		twait_block->vtag_block[0].v_tag = tag;
		twait_block->vtag_block[0].lport = lport;
		twait_block->vtag_block[0].rport = rport;
	}
}


#ifdef __Panda__
void panda_wakeup_socket(struct socket *so);
#endif









int
sctp_free_assoc(struct sctp_inpcb *inp, struct sctp_tcb *stcb, int from_inpcbfree, int from_location)
{
	int i;
	struct sctp_association *asoc;
	struct sctp_nets *net, *nnet;
	struct sctp_laddr *laddr, *naddr;
	struct sctp_tmit_chunk *chk, *nchk;
	struct sctp_asconf_addr *aparam, *naparam;
	struct sctp_asconf_ack *aack, *naack;
	struct sctp_stream_reset_list *strrst, *nstrrst;
	struct sctp_queued_to_read *sq, *nsq;
	struct sctp_stream_queue_pending *sp, *nsp;
	sctp_sharedkey_t *shared_key, *nshared_key;
	struct socket *so;

	
#if defined(__APPLE__)
	sctp_lock_assert(SCTP_INP_SO(inp));
#endif

#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, stcb, 6);
#endif
	if (stcb->asoc.state == 0) {
#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, NULL, 7);
#endif
		
		return (1);
	}
	if (stcb->asoc.alternate) {
		sctp_free_remote_addr(stcb->asoc.alternate);
		stcb->asoc.alternate = NULL;
	}
#if !defined(__APPLE__) 
        
	if (stcb->freed_from_where == 0) {
		
		stcb->freed_from_where = from_location;
	}
        
#endif

	asoc = &stcb->asoc;
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE))
		
		so = NULL;
	else
		so = inp->sctp_socket;

	




	if ((stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) &&
	    (from_inpcbfree == SCTP_NORMAL_PROC)) {
		



		if (stcb->asoc.refcnt) {
			
			sctp_timer_start(SCTP_TIMER_TYPE_ASOCKILL, inp, stcb, NULL);
			
			SCTP_TCB_UNLOCK(stcb);
#ifdef SCTP_LOG_CLOSING
			sctp_log_closing(inp, stcb, 8);
#endif
			return (0);
		}
	}
	
	(void)SCTP_OS_TIMER_STOP(&asoc->dack_timer.timer);
	asoc->dack_timer.self = NULL;
	(void)SCTP_OS_TIMER_STOP(&asoc->strreset_timer.timer);
	





	if (asoc->strreset_timer.type == SCTP_TIMER_TYPE_STRRESET)
		asoc->strreset_timer.self = NULL;
	(void)SCTP_OS_TIMER_STOP(&asoc->asconf_timer.timer);
	asoc->asconf_timer.self = NULL;
	(void)SCTP_OS_TIMER_STOP(&asoc->autoclose_timer.timer);
	asoc->autoclose_timer.self = NULL;
	(void)SCTP_OS_TIMER_STOP(&asoc->shut_guard_timer.timer);
	asoc->shut_guard_timer.self = NULL;
	(void)SCTP_OS_TIMER_STOP(&asoc->delayed_event_timer.timer);
	asoc->delayed_event_timer.self = NULL;
	
	(void)SCTP_OS_TIMER_STOP(&asoc->delete_prim_timer.timer);
	asoc->delete_prim_timer.self = NULL;
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		(void)SCTP_OS_TIMER_STOP(&net->rxt_timer.timer);
		net->rxt_timer.self = NULL;
		(void)SCTP_OS_TIMER_STOP(&net->pmtu_timer.timer);
		net->pmtu_timer.self = NULL;
		(void)SCTP_OS_TIMER_STOP(&net->hb_timer.timer);
		net->hb_timer.self = NULL;
	}
	
	if ((stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) == 0) {
		stcb->asoc.state |= SCTP_STATE_ABOUT_TO_BE_FREED;
		SCTP_INP_READ_LOCK(inp);
		TAILQ_FOREACH(sq, &inp->read_queue, next) {
			if (sq->stcb == stcb) {
				sq->do_not_ref_stcb = 1;
				sq->sinfo_cumtsn = stcb->asoc.cumulative_tsn;
				


				if (sq->end_added == 0) {
					
					sq->pdapi_aborted = 1;
					sq->held_length = 0;
					if (sctp_stcb_is_feature_on(inp, stcb, SCTP_PCB_FLAGS_PDAPIEVNT) && (so != NULL)) {
						




						uint32_t strseq;
						stcb->asoc.control_pdapi = sq;
						strseq = (sq->sinfo_stream << 16) | sq->sinfo_ssn;
						sctp_ulp_notify(SCTP_NOTIFY_PARTIAL_DELVIERY_INDICATION,
						                stcb,
						                SCTP_PARTIAL_DELIVERY_ABORTED,
						                (void *)&strseq,
						                SCTP_SO_LOCKED);
						stcb->asoc.control_pdapi = NULL;
					}
				}
				
				sq->end_added = 1;
			}
		}
		SCTP_INP_READ_UNLOCK(inp);
		if (stcb->block_entry) {
			SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_PCB, ECONNRESET);
			stcb->block_entry->error = ECONNRESET;
			stcb->block_entry = NULL;
		}
	}
	if ((stcb->asoc.refcnt) || (stcb->asoc.state & SCTP_STATE_IN_ACCEPT_QUEUE)) {
		

		if ((stcb->asoc.refcnt)  ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE)) {
			stcb->asoc.state &= ~SCTP_STATE_IN_ACCEPT_QUEUE;
			sctp_timer_start(SCTP_TIMER_TYPE_ASOCKILL, inp, stcb, NULL);
		}
		SCTP_TCB_UNLOCK(stcb);
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE))
			
			so = NULL;
		if (so) {
			
			sctp_sorwakeup(inp, so);
			sctp_sowwakeup(inp, so);
		}

#ifdef SCTP_LOG_CLOSING
		sctp_log_closing(inp, stcb, 9);
#endif
		
		return (0);
	}
#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, stcb, 10);
#endif
	














	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		atomic_add_int(&stcb->asoc.refcnt, 1);

		SCTP_TCB_UNLOCK(stcb);
		SCTP_INP_INFO_WLOCK();
		SCTP_INP_WLOCK(inp);
		SCTP_TCB_LOCK(stcb);
	}
	
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE))
		
		so = NULL;

	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
		



		if (inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) {
			inp->sctp_flags &= ~SCTP_PCB_FLAGS_CONNECTED;
			inp->sctp_flags |= SCTP_PCB_FLAGS_WAS_CONNECTED;
			if (so) {
				SOCK_LOCK(so);
				if (so->so_rcv.sb_cc == 0) {
					so->so_state &= ~(SS_ISCONNECTING |
							  SS_ISDISCONNECTING |
							  SS_ISCONFIRMING |
							  SS_ISCONNECTED);
				}
#if defined(__APPLE__)
				socantrcvmore(so);
#else
				socantrcvmore_locked(so);
#endif
				sctp_sowwakeup(inp, so);
				sctp_sorwakeup(inp, so);
				SCTP_SOWAKEUP(so);
			}
		}
	}

	


	
	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		atomic_add_int(&stcb->asoc.refcnt, -1);
	}
	if (stcb->asoc.refcnt) {
		stcb->asoc.state &= ~SCTP_STATE_IN_ACCEPT_QUEUE;
		sctp_timer_start(SCTP_TIMER_TYPE_ASOCKILL, inp, stcb, NULL);
		if (from_inpcbfree == SCTP_NORMAL_PROC) {
			SCTP_INP_INFO_WUNLOCK();
			SCTP_INP_WUNLOCK(inp);
		}
		SCTP_TCB_UNLOCK(stcb);
		return (0);
	}
	asoc->state = 0;
	if (inp->sctp_tcbhash) {
		LIST_REMOVE(stcb, sctp_tcbhash);
	}
	if (stcb->asoc.in_asocid_hash) {
		LIST_REMOVE(stcb, sctp_tcbasocidhash);
	}
	
	LIST_REMOVE(stcb, sctp_tcblist);
	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		SCTP_INP_INCR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
	}
	
	LIST_REMOVE(stcb, sctp_asocs);
	sctp_add_vtag_to_timewait(asoc->my_vtag, SCTP_BASE_SYSCTL(sctp_vtag_time_wait),
				  inp->sctp_lport, stcb->rport);

	


	(void)SCTP_OS_TIMER_STOP(&asoc->strreset_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->dack_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->strreset_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->asconf_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->shut_guard_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->autoclose_timer.timer);
	(void)SCTP_OS_TIMER_STOP(&asoc->delayed_event_timer.timer);
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		(void)SCTP_OS_TIMER_STOP(&net->rxt_timer.timer);
		(void)SCTP_OS_TIMER_STOP(&net->pmtu_timer.timer);
		(void)SCTP_OS_TIMER_STOP(&net->hb_timer.timer);
	}

	asoc->strreset_timer.type = SCTP_TIMER_TYPE_NONE;
	



	
	for (i = 0; i < asoc->streamoutcnt; i++) {
		struct sctp_stream_out *outs;

		outs = &asoc->strmout[i];
		
		TAILQ_FOREACH_SAFE(sp, &outs->outqueue, next, nsp) {
			TAILQ_REMOVE(&outs->outqueue, sp, next);
			sctp_free_spbufspace(stcb, asoc, sp);
			if (sp->data) {
				if (so) {
					
					sctp_ulp_notify(SCTP_NOTIFY_SPECIAL_SP_FAIL, stcb,
					                0, (void *)sp, SCTP_SO_LOCKED);
				}
				if (sp->data) {
					sctp_m_freem(sp->data);
					sp->data = NULL;
					sp->tail_mbuf = NULL;
					sp->length = 0;
				}
			}
			if (sp->net) {
				sctp_free_remote_addr(sp->net);
				sp->net = NULL;
			}
			sctp_free_a_strmoq(stcb, sp, SCTP_SO_LOCKED);
		}
	}
	
	TAILQ_FOREACH_SAFE(strrst, &asoc->resetHead, next_resp, nstrrst) {
		TAILQ_REMOVE(&asoc->resetHead, strrst, next_resp);
		SCTP_FREE(strrst, SCTP_M_STRESET);
	}
	TAILQ_FOREACH_SAFE(sq, &asoc->pending_reply_queue, next, nsq) {
		TAILQ_REMOVE(&asoc->pending_reply_queue, sq, next);
		if (sq->data) {
			sctp_m_freem(sq->data);
			sq->data = NULL;
		}
		sctp_free_remote_addr(sq->whoFrom);
		sq->whoFrom = NULL;
		sq->stcb = NULL;
		
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_readq), sq);
		SCTP_DECR_READQ_COUNT();
		
	}
	TAILQ_FOREACH_SAFE(chk, &asoc->free_chunks, sctp_next, nchk) {
		TAILQ_REMOVE(&asoc->free_chunks, chk, sctp_next);
		if (chk->data) {
			sctp_m_freem(chk->data);
			chk->data = NULL;
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		atomic_subtract_int(&SCTP_BASE_INFO(ipi_free_chunks), 1);
		asoc->free_chunk_cnt--;
		
	}
	
	TAILQ_FOREACH_SAFE(chk, &asoc->send_queue, sctp_next, nchk) {
		if (asoc->strmout[chk->rec.data.stream_number].chunks_on_queues > 0) {
			asoc->strmout[chk->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
		} else {
			panic("No chunks on the queues for sid %u.", chk->rec.data.stream_number);
#endif
		}
		TAILQ_REMOVE(&asoc->send_queue, chk, sctp_next);
		if (chk->data) {
			if (so) {
				
				sctp_ulp_notify(SCTP_NOTIFY_UNSENT_DG_FAIL, stcb,
				                0, chk, SCTP_SO_LOCKED);
			}
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		if (chk->whoTo) {
			sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = NULL;
		}
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		
	}
	
	TAILQ_FOREACH_SAFE(chk, &asoc->sent_queue, sctp_next, nchk) {
		if (chk->sent != SCTP_DATAGRAM_NR_ACKED) {
			if (asoc->strmout[chk->rec.data.stream_number].chunks_on_queues > 0) {
				asoc->strmout[chk->rec.data.stream_number].chunks_on_queues--;
#ifdef INVARIANTS
			} else {
				panic("No chunks on the queues for sid %u.", chk->rec.data.stream_number);
#endif
			}
		}
		TAILQ_REMOVE(&asoc->sent_queue, chk, sctp_next);
		if (chk->data) {
			if (so) {
				
				sctp_ulp_notify(SCTP_NOTIFY_SENT_DG_FAIL, stcb,
				                0, chk, SCTP_SO_LOCKED);
			}
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		sctp_free_remote_addr(chk->whoTo);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		
	}
#ifdef INVARIANTS
	for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
		if (stcb->asoc.strmout[i].chunks_on_queues > 0) {
			panic("%u chunks left for stream %u.", stcb->asoc.strmout[i].chunks_on_queues, i);
		}
	}
#endif
	
	TAILQ_FOREACH_SAFE(chk, &asoc->control_send_queue, sctp_next, nchk) {
		TAILQ_REMOVE(&asoc->control_send_queue, chk, sctp_next);
		if (chk->data) {
			sctp_m_freem(chk->data);
			chk->data = NULL;
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		sctp_free_remote_addr(chk->whoTo);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		
	}
	
	TAILQ_FOREACH_SAFE(chk, &asoc->asconf_send_queue, sctp_next, nchk) {
		TAILQ_REMOVE(&asoc->asconf_send_queue, chk, sctp_next);
		if (chk->data) {
			sctp_m_freem(chk->data);
			chk->data = NULL;
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		sctp_free_remote_addr(chk->whoTo);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		
	}
	TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
		TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
		if (chk->data) {
			sctp_m_freem(chk->data);
			chk->data = NULL;
		}
		if (chk->holds_key_ref)
			sctp_auth_key_release(stcb, chk->auth_keyid, SCTP_SO_LOCKED);
		sctp_free_remote_addr(chk->whoTo);
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_chunk), chk);
		SCTP_DECR_CHK_COUNT();
		
	}

	if (asoc->mapping_array) {
		SCTP_FREE(asoc->mapping_array, SCTP_M_MAP);
		asoc->mapping_array = NULL;
	}
	if (asoc->nr_mapping_array) {
		SCTP_FREE(asoc->nr_mapping_array, SCTP_M_MAP);
		asoc->nr_mapping_array = NULL;
	}
	
	if (asoc->strmout) {
		SCTP_FREE(asoc->strmout, SCTP_M_STRMO);
		asoc->strmout = NULL;
	}
	asoc->strm_realoutsize = asoc->streamoutcnt = 0;
	if (asoc->strmin) {
		struct sctp_queued_to_read *ctl, *nctl;

		for (i = 0; i < asoc->streamincnt; i++) {
			TAILQ_FOREACH_SAFE(ctl, &asoc->strmin[i].inqueue, next, nctl) {
				TAILQ_REMOVE(&asoc->strmin[i].inqueue, ctl, next);
				sctp_free_remote_addr(ctl->whoFrom);
				if (ctl->data) {
					sctp_m_freem(ctl->data);
					ctl->data = NULL;
				}
				




				SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_readq), ctl);
				SCTP_DECR_READQ_COUNT();
			}
		}
		SCTP_FREE(asoc->strmin, SCTP_M_STRMI);
		asoc->strmin = NULL;
	}
	asoc->streamincnt = 0;
	TAILQ_FOREACH_SAFE(net, &asoc->nets, sctp_next, nnet) {
#ifdef INVARIANTS
		if (SCTP_BASE_INFO(ipi_count_raddr) == 0) {
			panic("no net's left alloc'ed, or list points to itself");
		}
#endif
		TAILQ_REMOVE(&asoc->nets, net, sctp_next);
		sctp_free_remote_addr(net);
	}
	LIST_FOREACH_SAFE(laddr, &asoc->sctp_restricted_addrs, sctp_nxt_addr, naddr) {
		
		sctp_remove_laddr(laddr);
	}

	
	TAILQ_FOREACH_SAFE(aparam, &asoc->asconf_queue, next, naparam) {
		
		TAILQ_REMOVE(&asoc->asconf_queue, aparam, next);
		SCTP_FREE(aparam,SCTP_M_ASC_ADDR);
	}
	TAILQ_FOREACH_SAFE(aack, &asoc->asconf_ack_sent, next, naack) {
		
		TAILQ_REMOVE(&asoc->asconf_ack_sent, aack, next);
		if (aack->data != NULL) {
			sctp_m_freem(aack->data);
		}
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asconf_ack), aack);
	}
	
	if (asoc->local_hmacs)
		sctp_free_hmaclist(asoc->local_hmacs);
	if (asoc->peer_hmacs)
		sctp_free_hmaclist(asoc->peer_hmacs);

	if (asoc->local_auth_chunks)
		sctp_free_chunklist(asoc->local_auth_chunks);
	if (asoc->peer_auth_chunks)
		sctp_free_chunklist(asoc->peer_auth_chunks);

	sctp_free_authinfo(&asoc->authinfo);

	LIST_FOREACH_SAFE(shared_key, &asoc->shared_keys, next, nshared_key) {
		LIST_REMOVE(shared_key, next);
		sctp_free_sharedkey(shared_key);
		
	}

	

	
	SCTP_TCB_LOCK_DESTROY(stcb);
	SCTP_TCB_SEND_LOCK_DESTROY(stcb);
	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		SCTP_INP_INFO_WUNLOCK();
		SCTP_INP_RLOCK(inp);
	}
#if defined(__APPLE__) 
	stcb->freed_from_where = from_location;
#endif
#ifdef SCTP_TRACK_FREED_ASOCS
	if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
		
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), stcb);
		SCTP_DECR_ASOC_COUNT();
	} else {
		LIST_INSERT_HEAD(&inp->sctp_asoc_free_list, stcb, sctp_tcblist);
	}
#else
	SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_asoc), stcb);
	SCTP_DECR_ASOC_COUNT();
#endif
	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		if (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) {
			



			SCTP_INP_RUNLOCK(inp);
			






			sctp_inpcb_free(inp,
					SCTP_FREE_SHOULD_USE_GRACEFUL_CLOSE,
					SCTP_CALLED_DIRECTLY_NOCMPSET);
			SCTP_INP_DECR_REF(inp);
			goto out_of;
		} else {
			
			SCTP_INP_DECR_REF(inp);
		}
	}
	if (from_inpcbfree == SCTP_NORMAL_PROC) {
		SCTP_INP_RUNLOCK(inp);
	}
 out_of:
	
#ifdef SCTP_LOG_CLOSING
	sctp_log_closing(inp, NULL, 11);
#endif
	return (1);
}












int
sctp_destination_is_reachable(struct sctp_tcb *stcb, struct sockaddr *destaddr)
{
	struct sctp_inpcb *inp;
	int answer;

	









	inp = stcb->sctp_ep;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		
		




		return (1);
	}
	
	switch (destaddr->sa_family) {
#ifdef INET6
	case AF_INET6:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
		answer = inp->inp_vflag & INP_IPV6;
#else
		answer = inp->ip_inp.inp.inp_vflag & INP_IPV6;
#endif
		break;
#endif
#ifdef INET
	case AF_INET:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
		answer = inp->inp_vflag & INP_IPV4;
#else
		answer = inp->ip_inp.inp.inp_vflag & INP_IPV4;
#endif
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		answer = inp->ip_inp.inp.inp_vflag & INP_CONN;
		break;
#endif
	default:
		
		answer = 0;
		break;
	}
	return (answer);
}




static void
sctp_update_ep_vflag(struct sctp_inpcb *inp)
{
	struct sctp_laddr *laddr;

	
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
	inp->inp_vflag = 0;
#else
	inp->ip_inp.inp.inp_vflag = 0;
#endif
	
	LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
			SCTPDBG(SCTP_DEBUG_PCB1, "%s: NULL ifa\n",
				__FUNCTION__);
			continue;
		}

		if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED) {
			continue;
		}
		switch (laddr->ifa->address.sa.sa_family) {
#ifdef INET6
		case AF_INET6:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
			inp->inp_vflag |= INP_IPV6;
#else
			inp->ip_inp.inp.inp_vflag |= INP_IPV6;
#endif
			break;
#endif
#ifdef INET
		case AF_INET:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
			inp->inp_vflag |= INP_IPV4;
#else
			inp->ip_inp.inp.inp_vflag |= INP_IPV4;
#endif
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			inp->ip_inp.inp.inp_vflag |= INP_CONN;
			break;
#endif
		default:
			break;
		}
	}
}





void
sctp_add_local_addr_ep(struct sctp_inpcb *inp, struct sctp_ifa *ifa, uint32_t action)
{
	struct sctp_laddr *laddr;
	int fnd, error = 0;

	fnd = 0;

	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		
		return;
	}
#ifdef INET6
	if (ifa->address.sa.sa_family == AF_INET6) {
		if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
			
			return;
		}
	}
#endif
	
	LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == ifa) {
			fnd = 1;
			break;
		}
	}

	if (fnd == 0) {
		
		error = sctp_insert_laddr(&inp->sctp_addr_list, ifa, action);
		if (error != 0)
			return;
		inp->laddr_count++;
		
		switch (ifa->address.sa.sa_family) {
#ifdef INET6
		case AF_INET6:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
			inp->inp_vflag |= INP_IPV6;
#else
			inp->ip_inp.inp.inp_vflag |= INP_IPV6;
#endif
			break;
#endif
#ifdef INET
		case AF_INET:
#if !(defined(__FreeBSD__) || defined(__APPLE__) || defined(__Windows__) || defined(__Userspace__))
			inp->inp_vflag |= INP_IPV4;
#else
			inp->ip_inp.inp.inp_vflag |= INP_IPV4;
#endif
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			inp->ip_inp.inp.inp_vflag |= INP_CONN;
			break;
#endif
		default:
			break;
		}
	}
	return;
}







static void
sctp_select_primary_destination(struct sctp_tcb *stcb)
{
	struct sctp_nets *net;

	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		
		if (net->dest_state & SCTP_ADDR_UNCONFIRMED)
			continue;
		if (sctp_destination_is_reachable(stcb,
		    (struct sockaddr *)&net->ro._l_addr)) {
			
			stcb->asoc.primary_destination = net;
		}
	}
	
}






void
sctp_del_local_addr_ep(struct sctp_inpcb *inp, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;
	int fnd;

	fnd = 0;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		
		return;
	}
	LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == ifa) {
			fnd = 1;
			break;
		}
	}
	if (fnd && (inp->laddr_count < 2)) {
		
		return;
	}
	if (fnd) {
		





		struct sctp_tcb *stcb;

		
		if (inp->next_addr_touse == laddr)
			
			inp->next_addr_touse = NULL;

		
		LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
			struct sctp_nets *net;
			SCTP_TCB_LOCK(stcb);
			if (stcb->asoc.last_used_address == laddr)
				
				stcb->asoc.last_used_address = NULL;
			
			TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
				if (net->ro._s_addr &&
				    (net->ro._s_addr->ifa == laddr->ifa)) {
					
					sctp_rtentry_t *rt;

					
					rt = net->ro.ro_rt;
					if (rt != NULL) {
						RTFREE(rt);
						net->ro.ro_rt = NULL;
					}
					sctp_free_ifa(net->ro._s_addr);
					net->ro._s_addr = NULL;
					net->src_addr_selected = 0;
				}
			}
			SCTP_TCB_UNLOCK(stcb);
		}		
		
		sctp_remove_laddr(laddr);
		inp->laddr_count--;
		
		sctp_update_ep_vflag(inp);
	}
	return;
}






void
sctp_add_local_addr_restricted(struct sctp_tcb *stcb, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;
	struct sctpladdr *list;

	



	list = &stcb->asoc.sctp_restricted_addrs;

#ifdef INET6
	if (ifa->address.sa.sa_family == AF_INET6) {
		if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
			
			return;
		}
	}
#endif
	
	LIST_FOREACH(laddr, list, sctp_nxt_addr) {
		if (laddr->ifa == ifa) {
			return;
		}
	}

	
	(void)sctp_insert_laddr(list, ifa, 0);
	return;
}




int
sctp_insert_laddr(struct sctpladdr *list, struct sctp_ifa *ifa, uint32_t act)
{
	struct sctp_laddr *laddr;

	laddr = SCTP_ZONE_GET(SCTP_BASE_INFO(ipi_zone_laddr), struct sctp_laddr);
	if (laddr == NULL) {
		
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_PCB, EINVAL);
		return (EINVAL);
	}
	SCTP_INCR_LADDR_COUNT();
	bzero(laddr, sizeof(*laddr));
	(void)SCTP_GETTIME_TIMEVAL(&laddr->start_time);
	laddr->ifa = ifa;
	laddr->action = act;
	atomic_add_int(&ifa->refcount, 1);
	
	LIST_INSERT_HEAD(list, laddr, sctp_nxt_addr);

	return (0);
}




void
sctp_remove_laddr(struct sctp_laddr *laddr)
{

	
	LIST_REMOVE(laddr, sctp_nxt_addr);
	sctp_free_ifa(laddr->ifa);
	SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_laddr), laddr);
	SCTP_DECR_LADDR_COUNT();
}




void
sctp_del_local_addr_restricted(struct sctp_tcb *stcb, struct sctp_ifa *ifa)
{
	struct sctp_inpcb *inp;
	struct sctp_laddr *laddr;

	









	inp = stcb->sctp_ep;
	
	if (((inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) == 0) &&
	    sctp_is_feature_off(inp, SCTP_PCB_FLAGS_DO_ASCONF)) {
		if (stcb->sctp_ep->laddr_count < 2) {
			
			return;
		}
	}
	LIST_FOREACH(laddr, &stcb->asoc.sctp_restricted_addrs, sctp_nxt_addr) {
		
		if (laddr->ifa == NULL)
			continue;
		if (laddr->ifa == ifa) {
			sctp_remove_laddr(laddr);
			return;
		}
	}

	
	return;
}

#if defined(__FreeBSD__)




static int sctp_max_number_of_assoc = SCTP_MAX_NUM_OF_ASOC;
static int sctp_scale_up_for_address = SCTP_SCALE_FOR_ADDR;
#endif				



#if defined(__FreeBSD__) && defined(SCTP_MCORE_INPUT) && defined(SMP)
struct sctp_mcore_ctrl *sctp_mcore_workers = NULL;
int *sctp_cpuarry = NULL;
void
sctp_queue_to_mcore(struct mbuf *m, int off, int cpu_to_use)
{
	
	struct sctp_mcore_queue *qent;
	struct sctp_mcore_ctrl *wkq;
	int need_wake = 0;
	if (sctp_mcore_workers == NULL) {
		
		sctp_input_with_port(m, off, 0);
		return;
	}
	SCTP_MALLOC(qent, struct sctp_mcore_queue *,
		    (sizeof(struct sctp_mcore_queue)),
		    SCTP_M_MCORE);
	if (qent == NULL) {
		
		sctp_input_with_port(m, off, 0);
		return;
	}
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
	qent->vn = curvnet;
#endif
	qent->m = m;
	qent->off = off;
	qent->v6 = 0;
	wkq = &sctp_mcore_workers[cpu_to_use];
	SCTP_MCORE_QLOCK(wkq);

	TAILQ_INSERT_TAIL(&wkq->que, qent, next);
	if (wkq->running == 0) {
		need_wake = 1;
	}
	SCTP_MCORE_QUNLOCK(wkq);
	if (need_wake) {
		wakeup(&wkq->running);
	}
}

static void
sctp_mcore_thread(void *arg)
{

	struct sctp_mcore_ctrl *wkq;
	struct sctp_mcore_queue *qent;

	wkq = (struct sctp_mcore_ctrl *)arg;
	struct mbuf *m;
	int off, v6;

	
	SCTP_MCORE_LOCK(wkq);
	wkq->running = 0;
	msleep(&wkq->running,
	       &wkq->core_mtx,
	       0, "wait for pkt", 0);
	SCTP_MCORE_UNLOCK(wkq);

	
	thread_lock(curthread);
	sched_bind(curthread, wkq->cpuid);
	thread_unlock(curthread);

	
	SCTP_MCORE_LOCK(wkq);
	
	for (;;) {
		SCTP_MCORE_QLOCK(wkq);
	skip_sleep:
		wkq->running = 1;
		qent = TAILQ_FIRST(&wkq->que);
		if (qent) {
			TAILQ_REMOVE(&wkq->que, qent, next);
			SCTP_MCORE_QUNLOCK(wkq);
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
			CURVNET_SET(qent->vn);
#endif
			m = qent->m;
			off = qent->off;
			v6 = qent->v6;
			SCTP_FREE(qent, SCTP_M_MCORE);
			if (v6 == 0) {
				sctp_input_with_port(m, off, 0);
			} else {
				SCTP_PRINTF("V6 not yet supported\n");
				sctp_m_freem(m);
			}
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
			CURVNET_RESTORE();
#endif
			SCTP_MCORE_QLOCK(wkq);
		}
		wkq->running = 0;
		if (!TAILQ_EMPTY(&wkq->que)) {
			goto skip_sleep;
		}
		SCTP_MCORE_QUNLOCK(wkq);
		msleep(&wkq->running,
		       &wkq->core_mtx,
		       0, "wait for pkt", 0);
	}
}

static void
sctp_startup_mcore_threads(void)
{
	int i, cpu;

	if (mp_ncpus == 1)
		return;

	if (sctp_mcore_workers != NULL) {
		


		return;
	}
	SCTP_MALLOC(sctp_mcore_workers, struct sctp_mcore_ctrl *,
		    ((mp_maxid+1) * sizeof(struct sctp_mcore_ctrl)),
		    SCTP_M_MCORE);
	if (sctp_mcore_workers == NULL) {
		
		return;
	}
	memset(sctp_mcore_workers, 0 , ((mp_maxid+1) *
					sizeof(struct sctp_mcore_ctrl)));
	
	for (i = 0; i<=mp_maxid; i++) {
		TAILQ_INIT(&sctp_mcore_workers[i].que);
		SCTP_MCORE_LOCK_INIT(&sctp_mcore_workers[i]);
		SCTP_MCORE_QLOCK_INIT(&sctp_mcore_workers[i]);
		sctp_mcore_workers[i].cpuid = i;
	}
	if (sctp_cpuarry == NULL) {
		SCTP_MALLOC(sctp_cpuarry, int *,
			    (mp_ncpus * sizeof(int)),
			    SCTP_M_MCORE);
		i = 0;
		CPU_FOREACH(cpu) {
			sctp_cpuarry[i] = cpu;
			i++;
		}
	}

	
	CPU_FOREACH(cpu) {
#if __FreeBSD_version <= 701000
		(void)kthread_create(sctp_mcore_thread,
				     (void *)&sctp_mcore_workers[cpu],
				     &sctp_mcore_workers[cpu].thread_proc,
				     RFPROC,
				     SCTP_KTHREAD_PAGES,
				     SCTP_MCORE_NAME);

#else
		(void)kproc_create(sctp_mcore_thread,
				   (void *)&sctp_mcore_workers[cpu],
				   &sctp_mcore_workers[cpu].thread_proc,
				   RFPROC,
				   SCTP_KTHREAD_PAGES,
				   SCTP_MCORE_NAME);
#endif

	}
}
#endif
#if defined(__FreeBSD__) && __FreeBSD_cc_version >= 1100000
static struct mbuf *
sctp_netisr_hdlr(struct mbuf *m, uintptr_t source)
{
	struct ip *ip;
	struct sctphdr *sh;
	int offset;
	uint32_t flowid, tag;

	



	ip = mtod(m, struct ip *);
	offset = (ip->ip_hl << 2) + sizeof(struct sctphdr);
	if (SCTP_BUF_LEN(m) < offset) {
		if ((m = m_pullup(m, offset)) == NULL) {
			SCTP_STAT_INCR(sctps_hdrops);
			return (NULL);
		}
		ip = mtod(m, struct ip *);
	}
	sh = (struct sctphdr *)((caddr_t)ip + (ip->ip_hl << 2));
	tag = htonl(sh->v_tag);
	flowid = tag ^ ntohs(sh->dest_port) ^ ntohs(sh->src_port);
	m->m_pkthdr.flowid = flowid;
	m->m_flags |= M_FLOWID;
	return (m);
}
#endif

void
sctp_pcb_init()
{
	



	int i;
	struct timeval tv;

	if (SCTP_BASE_VAR(sctp_pcb_initialized) != 0) {
		
		return;
	}
	SCTP_BASE_VAR(sctp_pcb_initialized) = 1;

#if defined(SCTP_LOCAL_TRACE_BUF)
#if defined(__Windows__)
	if (SCTP_BASE_SYSCTL(sctp_log) != NULL) {
		bzero(SCTP_BASE_SYSCTL(sctp_log), sizeof(struct sctp_log));
	}
#else
	bzero(&SCTP_BASE_SYSCTL(sctp_log), sizeof(struct sctp_log));
#endif
#endif
#if defined(__FreeBSD__) && defined(SMP) && defined(SCTP_USE_PERCPU_STAT)
	SCTP_MALLOC(SCTP_BASE_STATS, struct sctpstat *,
		    ((mp_maxid+1) * sizeof(struct sctpstat)),
		    SCTP_M_MCORE);
#endif
	(void)SCTP_GETTIME_TIMEVAL(&tv);
#if defined(__FreeBSD__) && defined(SMP) && defined(SCTP_USE_PERCPU_STAT)
	bzero(SCTP_BASE_STATS, (sizeof(struct sctpstat) * (mp_maxid+1)));
	SCTP_BASE_STATS[PCPU_GET(cpuid)].sctps_discontinuitytime.tv_sec = (uint32_t)tv.tv_sec;
	SCTP_BASE_STATS[PCPU_GET(cpuid)].sctps_discontinuitytime.tv_usec = (uint32_t)tv.tv_usec;
#else
	bzero(&SCTP_BASE_STATS, sizeof(struct sctpstat));
	SCTP_BASE_STAT(sctps_discontinuitytime).tv_sec = (uint32_t)tv.tv_sec;
	SCTP_BASE_STAT(sctps_discontinuitytime).tv_usec = (uint32_t)tv.tv_usec;
#endif
	
	LIST_INIT(&SCTP_BASE_INFO(listhead));
#if defined(__APPLE__)
	LIST_INIT(&SCTP_BASE_INFO(inplisthead));
#endif


	
#if defined(__FreeBSD__)
#if defined(__FreeBSD_cc_version) && __FreeBSD_cc_version >= 440000
	TUNABLE_INT_FETCH("net.inet.sctp.tcbhashsize", &SCTP_BASE_SYSCTL(sctp_hashtblsize));
	TUNABLE_INT_FETCH("net.inet.sctp.pcbhashsize", &SCTP_BASE_SYSCTL(sctp_pcbtblsize));
	TUNABLE_INT_FETCH("net.inet.sctp.chunkscale", &SCTP_BASE_SYSCTL(sctp_chunkscale));
#else
	TUNABLE_INT_FETCH("net.inet.sctp.tcbhashsize", SCTP_TCBHASHSIZE,
			  SCTP_BASE_SYSCTL(sctp_hashtblsize));
	TUNABLE_INT_FETCH("net.inet.sctp.pcbhashsize", SCTP_PCBHASHSIZE,
			  SCTP_BASE_SYSCTL(sctp_pcbtblsize));
	TUNABLE_INT_FETCH("net.inet.sctp.chunkscale", SCTP_CHUNKQUEUE_SCALE,
			  SCTP_BASE_SYSCTL(sctp_chunkscale));
#endif
#endif
	SCTP_BASE_INFO(sctp_asochash) = SCTP_HASH_INIT((SCTP_BASE_SYSCTL(sctp_hashtblsize) * 31),
						       &SCTP_BASE_INFO(hashasocmark));
	SCTP_BASE_INFO(sctp_ephash) = SCTP_HASH_INIT(SCTP_BASE_SYSCTL(sctp_hashtblsize),
						     &SCTP_BASE_INFO(hashmark));
	SCTP_BASE_INFO(sctp_tcpephash) = SCTP_HASH_INIT(SCTP_BASE_SYSCTL(sctp_hashtblsize),
							&SCTP_BASE_INFO(hashtcpmark));
	SCTP_BASE_INFO(hashtblsize) = SCTP_BASE_SYSCTL(sctp_hashtblsize);


	SCTP_BASE_INFO(sctp_vrfhash) = SCTP_HASH_INIT(SCTP_SIZE_OF_VRF_HASH,
						      &SCTP_BASE_INFO(hashvrfmark));

	SCTP_BASE_INFO(vrf_ifn_hash) = SCTP_HASH_INIT(SCTP_VRF_IFN_HASH_SIZE,
						      &SCTP_BASE_INFO(vrf_ifn_hashmark));
	
	



	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_ep), "sctp_ep",
		       sizeof(struct sctp_inpcb), maxsockets);

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_asoc), "sctp_asoc",
		       sizeof(struct sctp_tcb), sctp_max_number_of_assoc);

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_laddr), "sctp_laddr",
		       sizeof(struct sctp_laddr),
		       (sctp_max_number_of_assoc * sctp_scale_up_for_address));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_net), "sctp_raddr",
		       sizeof(struct sctp_nets),
		       (sctp_max_number_of_assoc * sctp_scale_up_for_address));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_chunk), "sctp_chunk",
		       sizeof(struct sctp_tmit_chunk),
		       (sctp_max_number_of_assoc * SCTP_BASE_SYSCTL(sctp_chunkscale)));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_readq), "sctp_readq",
		       sizeof(struct sctp_queued_to_read),
		       (sctp_max_number_of_assoc * SCTP_BASE_SYSCTL(sctp_chunkscale)));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_strmoq), "sctp_stream_msg_out",
		       sizeof(struct sctp_stream_queue_pending),
		       (sctp_max_number_of_assoc * SCTP_BASE_SYSCTL(sctp_chunkscale)));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_asconf), "sctp_asconf",
		       sizeof(struct sctp_asconf),
		       (sctp_max_number_of_assoc * SCTP_BASE_SYSCTL(sctp_chunkscale)));

	SCTP_ZONE_INIT(SCTP_BASE_INFO(ipi_zone_asconf_ack), "sctp_asconf_ack",
		       sizeof(struct sctp_asconf_ack),
		       (sctp_max_number_of_assoc * SCTP_BASE_SYSCTL(sctp_chunkscale)));


	
#if defined(__APPLE__)
	
	SCTP_BASE_INFO(mtx_grp_attr) = lck_grp_attr_alloc_init();
	lck_grp_attr_setdefault(SCTP_BASE_INFO(mtx_grp_attr));
	
	SCTP_BASE_INFO(mtx_grp) = lck_grp_alloc_init("sctppcb",
						     SCTP_BASE_INFO(mtx_grp_attr));
	
	SCTP_BASE_INFO(mtx_attr) = lck_attr_alloc_init();
	lck_attr_setdefault(SCTP_BASE_INFO(mtx_attr));
#endif				
	SCTP_INP_INFO_LOCK_INIT();
	SCTP_STATLOG_INIT_LOCK();

	SCTP_IPI_COUNT_INIT();
	SCTP_IPI_ADDR_INIT();
#ifdef SCTP_PACKET_LOGGING
	SCTP_IP_PKTLOG_INIT();
#endif
	LIST_INIT(&SCTP_BASE_INFO(addr_wq));

	SCTP_WQ_ADDR_INIT();
	
	SCTP_BASE_INFO(ipi_count_ep) = 0;
	
	SCTP_BASE_INFO(ipi_count_asoc) = 0;
	
	SCTP_BASE_INFO(ipi_count_laddr) = 0;
	
	SCTP_BASE_INFO(ipi_count_raddr) = 0;
	
	SCTP_BASE_INFO(ipi_count_chunk) = 0;

	
	SCTP_BASE_INFO(ipi_count_readq) = 0;

	
	SCTP_BASE_INFO(ipi_count_strmoq) = 0;

	SCTP_BASE_INFO(ipi_free_strmoq) = 0;
	SCTP_BASE_INFO(ipi_free_chunks) = 0;

	SCTP_OS_TIMER_INIT(&SCTP_BASE_INFO(addr_wq_timer.timer));

	
	for (i = 0; i < SCTP_STACK_VTAG_HASH_SIZE; i++) {
		LIST_INIT(&SCTP_BASE_INFO(vtag_timewait)[i]);
	}

#if defined(SCTP_PROCESS_LEVEL_LOCKS)
#if defined(__Userspace_os_Windows)
	InitializeConditionVariable(&sctp_it_ctl.iterator_wakeup);
#else
	(void)pthread_cond_init(&sctp_it_ctl.iterator_wakeup, NULL);
#endif
#endif
	sctp_startup_iterator();

#if defined(__FreeBSD__) && defined(SCTP_MCORE_INPUT) && defined(SMP)
	sctp_startup_mcore_threads();
#endif

#ifndef __Panda__
	




	sctp_init_vrf_list(SCTP_DEFAULT_VRF);
#endif
#if defined(__FreeBSD__) && __FreeBSD_cc_version >= 1100000
	if (ip_register_flow_handler(sctp_netisr_hdlr, IPPROTO_SCTP)) {
		SCTP_PRINTF("***SCTP- Error can't register netisr handler***\n");
	}
#endif
#if defined(_SCTP_NEEDS_CALLOUT_) || defined(_USER_SCTP_NEEDS_CALLOUT_)
	
	SCTP_TIMERQ_LOCK_INIT();
	TAILQ_INIT(&SCTP_BASE_INFO(callqueue));
#endif
#if defined(__Userspace__)
	mbuf_init(NULL);
	atomic_init();
#if defined(INET) || defined(INET6)
	recv_thread_init();
#endif
#endif
}




void
sctp_pcb_finish(void)
{
	struct sctp_vrflist *vrf_bucket;
	struct sctp_vrf *vrf, *nvrf;
	struct sctp_ifn *ifn, *nifn;
	struct sctp_ifa *ifa, *nifa;
	struct sctpvtaghead *chain;
	struct sctp_tagblock *twait_block, *prev_twait_block;
	struct sctp_laddr *wi, *nwi;
	int i;

#if defined(__FreeBSD__)
	





	{
		struct sctp_iterator *it, *nit;

		SCTP_IPI_ITERATOR_WQ_LOCK();
		TAILQ_FOREACH_SAFE(it, &sctp_it_ctl.iteratorhead, sctp_nxt_itr, nit) {
			if (it->vn != curvnet) {
				continue;
			}
			TAILQ_REMOVE(&sctp_it_ctl.iteratorhead, it, sctp_nxt_itr);
			if (it->function_atend != NULL) {
				(*it->function_atend) (it->pointer, it->val);
			}
			SCTP_FREE(it,SCTP_M_ITER);
		}
		SCTP_IPI_ITERATOR_WQ_UNLOCK();
		SCTP_ITERATOR_LOCK();
		if ((sctp_it_ctl.cur_it) &&
		    (sctp_it_ctl.cur_it->vn == curvnet)) {
			sctp_it_ctl.iterator_flags |= SCTP_ITERATOR_STOP_CUR_IT;
		}
		SCTP_ITERATOR_UNLOCK();
	}
#else
	
	SCTP_IPI_ITERATOR_WQ_LOCK();
	sctp_it_ctl.iterator_flags |= SCTP_ITERATOR_MUST_EXIT;
	sctp_wakeup_iterator();
	SCTP_IPI_ITERATOR_WQ_UNLOCK();
#endif
#if defined(__APPLE__)
	SCTP_IPI_ITERATOR_WQ_LOCK();
	do {
		msleep(&sctp_it_ctl.iterator_flags,
		       sctp_it_ctl.ipi_iterator_wq_mtx,
		       0, "waiting_for_work", 0);
	} while ((sctp_it_ctl.iterator_flags & SCTP_ITERATOR_EXITED) == 0);
	thread_deallocate(sctp_it_ctl.thread_proc);
	SCTP_IPI_ITERATOR_WQ_UNLOCK();
	SCTP_IPI_ITERATOR_WQ_DESTROY();
	SCTP_ITERATOR_LOCK_DESTROY();
#endif
#if defined(__Windows__)
	if (sctp_it_ctl.iterator_thread_obj != NULL) {
		NTSTATUS status = STATUS_SUCCESS;

		KeSetEvent(&sctp_it_ctl.iterator_wakeup[1], IO_NO_INCREMENT, FALSE);
		status = KeWaitForSingleObject(sctp_it_ctl.iterator_thread_obj,
					       Executive,
					       KernelMode,
					       FALSE,
					       NULL);
		ObDereferenceObject(sctp_it_ctl.iterator_thread_obj);
	}
#endif

	SCTP_OS_TIMER_STOP(&SCTP_BASE_INFO(addr_wq_timer.timer));
	SCTP_WQ_ADDR_LOCK();
	LIST_FOREACH_SAFE(wi, &SCTP_BASE_INFO(addr_wq), sctp_nxt_addr, nwi) {
		LIST_REMOVE(wi, sctp_nxt_addr);
		SCTP_DECR_LADDR_COUNT();
		SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_laddr), wi);
	}
	SCTP_WQ_ADDR_UNLOCK();

	



	vrf_bucket = &SCTP_BASE_INFO(sctp_vrfhash)[(SCTP_DEFAULT_VRFID & SCTP_BASE_INFO(hashvrfmark))];
	LIST_FOREACH_SAFE(vrf, vrf_bucket, next_vrf, nvrf) {
		LIST_FOREACH_SAFE(ifn, &vrf->ifnlist, next_ifn, nifn) {
			LIST_FOREACH_SAFE(ifa, &ifn->ifalist, next_ifa, nifa) {
				
				LIST_REMOVE(ifa, next_bucket);
				LIST_REMOVE(ifa, next_ifa);
				SCTP_FREE(ifa, SCTP_M_IFA);
			}
			
			LIST_REMOVE(ifn, next_bucket);
			LIST_REMOVE(ifn, next_ifn);
			SCTP_FREE(ifn, SCTP_M_IFN);
		}
		SCTP_HASH_FREE(vrf->vrf_addr_hash, vrf->vrf_addr_hashmark);
		
		LIST_REMOVE(vrf, next_vrf);
		SCTP_FREE(vrf, SCTP_M_VRF);
	}
	
	SCTP_HASH_FREE(SCTP_BASE_INFO(sctp_vrfhash), SCTP_BASE_INFO(hashvrfmark));
	SCTP_HASH_FREE(SCTP_BASE_INFO(vrf_ifn_hash), SCTP_BASE_INFO(vrf_ifn_hashmark));
#if defined(__Userspace__) && !defined(__Userspace_os_Windows)
	
	freeifaddrs(g_interfaces);
#endif

	


	for (i = 0; i < SCTP_STACK_VTAG_HASH_SIZE; i++) {
		chain = &SCTP_BASE_INFO(vtag_timewait)[i];
		if (!LIST_EMPTY(chain)) {
			prev_twait_block = NULL;
			LIST_FOREACH(twait_block, chain, sctp_nxt_tagblock) {
				if (prev_twait_block) {
					SCTP_FREE(prev_twait_block, SCTP_M_TIMW);
				}
				prev_twait_block = twait_block;
			}
			SCTP_FREE(prev_twait_block, SCTP_M_TIMW);
		}
	}

	
#if defined(__APPLE__)
	SCTP_TIMERQ_LOCK_DESTROY();
#endif
#ifdef SCTP_PACKET_LOGGING
	SCTP_IP_PKTLOG_DESTROY();
#endif
	SCTP_IPI_ADDR_DESTROY();
#if defined(__APPLE__)
	SCTP_IPI_COUNT_DESTROY();
#endif
	SCTP_STATLOG_DESTROY();
#if !defined(__Userspace__)
	SCTP_INP_INFO_LOCK_DESTROY();
#endif

	SCTP_WQ_ADDR_DESTROY();

#if defined(__APPLE__)
	lck_grp_attr_free(SCTP_BASE_INFO(mtx_grp_attr));
	lck_grp_free(SCTP_BASE_INFO(mtx_grp));
	lck_attr_free(SCTP_BASE_INFO(mtx_attr));
#endif
#if defined(__Userspace__)
	SCTP_TIMERQ_LOCK_DESTROY();
	SCTP_ZONE_DESTROY(zone_mbuf);
	SCTP_ZONE_DESTROY(zone_clust);
	SCTP_ZONE_DESTROY(zone_ext_refcnt);
#endif
#if defined(__Windows__) || defined(__FreeBSD__) || defined(__Userspace__)
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_ep));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_asoc));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_laddr));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_net));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_chunk));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_readq));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_strmoq));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_asconf));
	SCTP_ZONE_DESTROY(SCTP_BASE_INFO(ipi_zone_asconf_ack));
#endif
	
	if (SCTP_BASE_INFO(sctp_asochash) != NULL)
		SCTP_HASH_FREE(SCTP_BASE_INFO(sctp_asochash), SCTP_BASE_INFO(hashasocmark));
	if (SCTP_BASE_INFO(sctp_ephash) != NULL)
		SCTP_HASH_FREE(SCTP_BASE_INFO(sctp_ephash), SCTP_BASE_INFO(hashmark));
	if (SCTP_BASE_INFO(sctp_tcpephash) != NULL)
		SCTP_HASH_FREE(SCTP_BASE_INFO(sctp_tcpephash), SCTP_BASE_INFO(hashtcpmark));
#if defined(__FreeBSD__) && defined(SMP) && defined(SCTP_USE_PERCPU_STAT)
	SCTP_FREE(SCTP_BASE_STATS, SCTP_M_MCORE);
#endif
}


int
sctp_load_addresses_from_init(struct sctp_tcb *stcb, struct mbuf *m,
                              int offset, int limit,
                              struct sockaddr *src, struct sockaddr *dst,
                              struct sockaddr *altsa)
{
	






	struct sctp_inpcb *inp;
	struct sctp_nets *net, *nnet, *net_tmp;
	struct sctp_paramhdr *phdr, parm_buf;
	struct sctp_tcb *stcb_tmp;
	uint16_t ptype, plen;
	struct sockaddr *sa;
	uint8_t random_store[SCTP_PARAM_BUFFER_SIZE];
	struct sctp_auth_random *p_random = NULL;
	uint16_t random_len = 0;
	uint8_t hmacs_store[SCTP_PARAM_BUFFER_SIZE];
	struct sctp_auth_hmac_algo *hmacs = NULL;
	uint16_t hmacs_len = 0;
	uint8_t saw_asconf = 0;
	uint8_t saw_asconf_ack = 0;
	uint8_t chunks_store[SCTP_PARAM_BUFFER_SIZE];
	struct sctp_auth_chunk_list *chunks = NULL;
	uint16_t num_chunks = 0;
	sctp_key_t *new_key;
	uint32_t keylen;
	int got_random = 0, got_hmacs = 0, got_chklist = 0;
	uint8_t ecn_allowed;
#ifdef INET
	struct sockaddr_in sin;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6;
#endif

	
#ifdef INET
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	sin.sin_len = sizeof(sin);
#endif
	sin.sin_port = stcb->rport;
#endif
#ifdef INET6
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
	sin6.sin6_port = stcb->rport;
#endif
	if (altsa) {
		sa = altsa;
	} else {
		sa = src;
	}
	
	ecn_allowed = 0;
	TAILQ_FOREACH(net, &stcb->asoc.nets, sctp_next) {
		
		net->dest_state |= SCTP_ADDR_NOT_IN_ASSOC;
	}
	
	inp = stcb->sctp_ep;
	atomic_add_int(&stcb->asoc.refcnt, 1);
	stcb_tmp = sctp_findassociation_ep_addr(&inp, sa, &net_tmp, dst, stcb);
	atomic_add_int(&stcb->asoc.refcnt, -1);

	if ((stcb_tmp == NULL && inp == stcb->sctp_ep) || inp == NULL) {
		
		
		switch (sa->sa_family) {
#ifdef INET
		case AF_INET:
			if (stcb->asoc.scope.ipv4_addr_legal) {
				if (sctp_add_remote_addr(stcb, sa, NULL, SCTP_DONOT_SETSCOPE, SCTP_LOAD_ADDR_2)) {
					return (-1);
				}
			}
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (stcb->asoc.scope.ipv6_addr_legal) {
				if (sctp_add_remote_addr(stcb, sa, NULL, SCTP_DONOT_SETSCOPE, SCTP_LOAD_ADDR_3)) {
					return (-2);
				}
			}
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			if (stcb->asoc.scope.conn_addr_legal) {
				if (sctp_add_remote_addr(stcb, sa, NULL, SCTP_DONOT_SETSCOPE, SCTP_LOAD_ADDR_3)) {
					return (-2);
				}
			}
			break;
#endif
		default:
			break;
		}
	} else {
		if (net_tmp != NULL && stcb_tmp == stcb) {
			net_tmp->dest_state &= ~SCTP_ADDR_NOT_IN_ASSOC;
		} else if (stcb_tmp != stcb) {
			
			if (stcb_tmp)
				SCTP_TCB_UNLOCK(stcb_tmp);
			return (-3);
		}
	}
	if (stcb->asoc.state == 0) {
		
		return (-4);
	}
	




	stcb->asoc.peer_supports_asconf = 0;
	
	phdr = sctp_get_next_param(m, offset, &parm_buf, sizeof(parm_buf));
	while (phdr) {
		ptype = ntohs(phdr->param_type);
		plen = ntohs(phdr->param_length);
		



		if (offset + plen > limit) {
			break;
		}
		if (plen == 0) {
			break;
		}
#ifdef INET
		if (ptype == SCTP_IPV4_ADDRESS) {
			if (stcb->asoc.scope.ipv4_addr_legal) {
				struct sctp_ipv4addr_param *p4, p4_buf;

				
				phdr = sctp_get_next_param(m, offset,
							   (struct sctp_paramhdr *)&p4_buf,
							   sizeof(p4_buf));
				if (plen != sizeof(struct sctp_ipv4addr_param) ||
				    phdr == NULL) {
					return (-5);
				}
				p4 = (struct sctp_ipv4addr_param *)phdr;
				sin.sin_addr.s_addr = p4->addr;
				if (IN_MULTICAST(ntohl(sin.sin_addr.s_addr))) {
					
					goto next_param;
				}
				if ((sin.sin_addr.s_addr == INADDR_BROADCAST) ||
				    (sin.sin_addr.s_addr == INADDR_ANY)) {
					goto next_param;
				}
				sa = (struct sockaddr *)&sin;
				inp = stcb->sctp_ep;
				atomic_add_int(&stcb->asoc.refcnt, 1);
				stcb_tmp = sctp_findassociation_ep_addr(&inp, sa, &net,
									dst, stcb);
				atomic_add_int(&stcb->asoc.refcnt, -1);

				if ((stcb_tmp == NULL && inp == stcb->sctp_ep) ||
				    inp == NULL) {
					
					




					



				add_it_now:
					if (stcb->asoc.state == 0) {
						
						return (-7);
					}
					if (sctp_add_remote_addr(stcb, sa, NULL, SCTP_DONOT_SETSCOPE, SCTP_LOAD_ADDR_4)) {
						return (-8);
					}
				} else if (stcb_tmp == stcb) {
					if (stcb->asoc.state == 0) {
						
						return (-10);
					}
					if (net != NULL) {
						
						net->dest_state &=
							~SCTP_ADDR_NOT_IN_ASSOC;
					}
				} else {
					



					if (stcb_tmp) {
						if (SCTP_GET_STATE(&stcb_tmp->asoc) & SCTP_STATE_COOKIE_WAIT) {
							
							sctp_abort_an_association(stcb_tmp->sctp_ep,
										  stcb_tmp, NULL, SCTP_SO_NOT_LOCKED);
							goto add_it_now;
						}
						SCTP_TCB_UNLOCK(stcb_tmp);
					}

					if (stcb->asoc.state == 0) {
						
						return (-12);
					}
					return (-13);
				}
			}
		} else
#endif
#ifdef INET6
		if (ptype == SCTP_IPV6_ADDRESS) {
			if (stcb->asoc.scope.ipv6_addr_legal) {
				
				struct sctp_ipv6addr_param *p6, p6_buf;

				phdr = sctp_get_next_param(m, offset,
							   (struct sctp_paramhdr *)&p6_buf,
							   sizeof(p6_buf));
				if (plen != sizeof(struct sctp_ipv6addr_param) ||
				    phdr == NULL) {
					return (-14);
				}
				p6 = (struct sctp_ipv6addr_param *)phdr;
				memcpy((caddr_t)&sin6.sin6_addr, p6->addr,
				       sizeof(p6->addr));
				if (IN6_IS_ADDR_MULTICAST(&sin6.sin6_addr)) {
					
					goto next_param;
				}
				if (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)) {
					
					goto next_param;
				}
				sa = (struct sockaddr *)&sin6;
				inp = stcb->sctp_ep;
				atomic_add_int(&stcb->asoc.refcnt, 1);
				stcb_tmp = sctp_findassociation_ep_addr(&inp, sa, &net,
									dst, stcb);
				atomic_add_int(&stcb->asoc.refcnt, -1);
				if (stcb_tmp == NULL &&
				    (inp == stcb->sctp_ep || inp == NULL)) {
					



				add_it_now6:
					if (stcb->asoc.state == 0) {
						
						return (-16);
					}
					



					if (sctp_add_remote_addr(stcb, sa, NULL, SCTP_DONOT_SETSCOPE, SCTP_LOAD_ADDR_5)) {
						return (-17);
					}
				} else if (stcb_tmp == stcb) {
					



					if (stcb->asoc.state == 0) {
						
						return (-19);
					}
					if (net != NULL) {
						
						net->dest_state &=
							~SCTP_ADDR_NOT_IN_ASSOC;
					}
				} else {
					



					if (stcb_tmp)
						if (SCTP_GET_STATE(&stcb_tmp->asoc) & SCTP_STATE_COOKIE_WAIT) {
							
							sctp_abort_an_association(stcb_tmp->sctp_ep,
										  stcb_tmp, NULL, SCTP_SO_NOT_LOCKED);
							goto add_it_now6;
						}
					SCTP_TCB_UNLOCK(stcb_tmp);

					if (stcb->asoc.state == 0) {
						
						return (-21);
					}
					return (-22);
				}
			}
		} else
#endif
		if (ptype == SCTP_ECN_CAPABLE) {
			ecn_allowed = 1;
		} else if (ptype == SCTP_ULP_ADAPTATION) {
			if (stcb->asoc.state != SCTP_STATE_OPEN) {
				struct sctp_adaptation_layer_indication ai, *aip;

				phdr = sctp_get_next_param(m, offset,
							   (struct sctp_paramhdr *)&ai, sizeof(ai));
				aip = (struct sctp_adaptation_layer_indication *)phdr;
				if (aip) {
					stcb->asoc.peers_adaptation = ntohl(aip->indication);
					stcb->asoc.adaptation_needed = 1;
				}
			}
		} else if (ptype == SCTP_SET_PRIM_ADDR) {
			struct sctp_asconf_addr_param lstore, *fee;
			int lptype;
			struct sockaddr *lsa = NULL;
#ifdef INET
			struct sctp_asconf_addrv4_param *fii;
#endif

			stcb->asoc.peer_supports_asconf = 1;
			if (plen > sizeof(lstore)) {
				return (-23);
			}
			phdr = sctp_get_next_param(m, offset,
						   (struct sctp_paramhdr *)&lstore,
						   min(plen,sizeof(lstore)));
			if (phdr == NULL) {
				return (-24);
			}
			fee = (struct sctp_asconf_addr_param *)phdr;
			lptype = ntohs(fee->addrp.ph.param_type);
			switch (lptype) {
#ifdef INET
			case SCTP_IPV4_ADDRESS:
				if (plen !=
				    sizeof(struct sctp_asconf_addrv4_param)) {
					SCTP_PRINTF("Sizeof setprim in init/init ack not %d but %d - ignored\n",
						    (int)sizeof(struct sctp_asconf_addrv4_param),
						    plen);
				} else {
					fii = (struct sctp_asconf_addrv4_param *)fee;
					sin.sin_addr.s_addr = fii->addrp.addr;
					lsa = (struct sockaddr *)&sin;
				}
				break;
#endif
#ifdef INET6
			case SCTP_IPV6_ADDRESS:
				if (plen !=
				    sizeof(struct sctp_asconf_addr_param)) {
					SCTP_PRINTF("Sizeof setprim (v6) in init/init ack not %d but %d - ignored\n",
						    (int)sizeof(struct sctp_asconf_addr_param),
						    plen);
				} else {
					memcpy(sin6.sin6_addr.s6_addr,
					       fee->addrp.addr,
					       sizeof(fee->addrp.addr));
					lsa = (struct sockaddr *)&sin6;
				}
				break;
#endif
			default:
				break;
			}
			if (lsa) {
				(void)sctp_set_primary_addr(stcb, sa, NULL);
			}
		} else if (ptype == SCTP_HAS_NAT_SUPPORT) {
			stcb->asoc.peer_supports_nat = 1;
		} else if (ptype == SCTP_PRSCTP_SUPPORTED) {
			
			stcb->asoc.peer_supports_prsctp = 1;
		} else if (ptype == SCTP_SUPPORTED_CHUNK_EXT) {
			
			struct sctp_supported_chunk_types_param *pr_supported;
			uint8_t local_store[SCTP_PARAM_BUFFER_SIZE];
			int num_ent, i;

			phdr = sctp_get_next_param(m, offset,
						   (struct sctp_paramhdr *)&local_store, min(sizeof(local_store),plen));
			if (phdr == NULL) {
				return (-25);
			}
			stcb->asoc.peer_supports_asconf = 0;
			stcb->asoc.peer_supports_prsctp = 0;
			stcb->asoc.peer_supports_pktdrop = 0;
			stcb->asoc.peer_supports_strreset = 0;
			stcb->asoc.peer_supports_nr_sack = 0;
			stcb->asoc.peer_supports_auth = 0;
			pr_supported = (struct sctp_supported_chunk_types_param *)phdr;
			num_ent = plen - sizeof(struct sctp_paramhdr);
			for (i = 0; i < num_ent; i++) {
				switch (pr_supported->chunk_types[i]) {
				case SCTP_ASCONF:
				case SCTP_ASCONF_ACK:
					stcb->asoc.peer_supports_asconf = 1;
					break;
				case SCTP_FORWARD_CUM_TSN:
					stcb->asoc.peer_supports_prsctp = 1;
					break;
				case SCTP_PACKET_DROPPED:
					stcb->asoc.peer_supports_pktdrop = 1;
					break;
				case SCTP_NR_SELECTIVE_ACK:
					stcb->asoc.peer_supports_nr_sack = 1;
					break;
				case SCTP_STREAM_RESET:
					stcb->asoc.peer_supports_strreset = 1;
					break;
				case SCTP_AUTHENTICATION:
					stcb->asoc.peer_supports_auth = 1;
					break;
				default:
					
					break;

				}
			}
		} else if (ptype == SCTP_RANDOM) {
			if (plen > sizeof(random_store))
				break;
			if (got_random) {
				
				goto next_param;
			}
			phdr = sctp_get_next_param(m, offset,
						   (struct sctp_paramhdr *)random_store,
						   min(sizeof(random_store),plen));
			if (phdr == NULL)
				return (-26);
			p_random = (struct sctp_auth_random *)phdr;
			random_len = plen - sizeof(*p_random);
			
			if (random_len != SCTP_AUTH_RANDOM_SIZE_REQUIRED) {
				SCTPDBG(SCTP_DEBUG_AUTH1, "SCTP: invalid RANDOM len\n");
				return (-27);
			}
			got_random = 1;
		} else if (ptype == SCTP_HMAC_LIST) {
			int num_hmacs;
			int i;

			if (plen > sizeof(hmacs_store))
				break;
			if (got_hmacs) {
				
				goto next_param;
			}
			phdr = sctp_get_next_param(m, offset,
						   (struct sctp_paramhdr *)hmacs_store,
						   min(plen,sizeof(hmacs_store)));
			if (phdr == NULL)
				return (-28);
			hmacs = (struct sctp_auth_hmac_algo *)phdr;
			hmacs_len = plen - sizeof(*hmacs);
			num_hmacs = hmacs_len / sizeof(hmacs->hmac_ids[0]);
			
			if (sctp_verify_hmac_param(hmacs, num_hmacs)) {
				return (-29);
			}
			if (stcb->asoc.peer_hmacs != NULL)
				sctp_free_hmaclist(stcb->asoc.peer_hmacs);
			stcb->asoc.peer_hmacs = sctp_alloc_hmaclist(num_hmacs);
			if (stcb->asoc.peer_hmacs != NULL) {
				for (i = 0; i < num_hmacs; i++) {
					(void)sctp_auth_add_hmacid(stcb->asoc.peer_hmacs,
								   ntohs(hmacs->hmac_ids[i]));
				}
			}
			got_hmacs = 1;
		} else if (ptype == SCTP_CHUNK_LIST) {
			int i;

			if (plen > sizeof(chunks_store))
				break;
			if (got_chklist) {
				
				goto next_param;
			}
			phdr = sctp_get_next_param(m, offset,
						   (struct sctp_paramhdr *)chunks_store,
						   min(plen,sizeof(chunks_store)));
			if (phdr == NULL)
				return (-30);
			chunks = (struct sctp_auth_chunk_list *)phdr;
			num_chunks = plen - sizeof(*chunks);
			if (stcb->asoc.peer_auth_chunks != NULL)
				sctp_clear_chunklist(stcb->asoc.peer_auth_chunks);
			else
				stcb->asoc.peer_auth_chunks = sctp_alloc_chunklist();
			for (i = 0; i < num_chunks; i++) {
				(void)sctp_auth_add_chunk(chunks->chunk_types[i],
							  stcb->asoc.peer_auth_chunks);
				
				if (chunks->chunk_types[i] == SCTP_ASCONF)
					saw_asconf = 1;
				if (chunks->chunk_types[i] == SCTP_ASCONF_ACK)
					saw_asconf_ack = 1;

			}
			got_chklist = 1;
		} else if ((ptype == SCTP_HEARTBEAT_INFO) ||
			   (ptype == SCTP_STATE_COOKIE) ||
			   (ptype == SCTP_UNRECOG_PARAM) ||
			   (ptype == SCTP_COOKIE_PRESERVE) ||
			   (ptype == SCTP_SUPPORTED_ADDRTYPE) ||
			   (ptype == SCTP_ADD_IP_ADDRESS) ||
			   (ptype == SCTP_DEL_IP_ADDRESS) ||
			   (ptype == SCTP_ERROR_CAUSE_IND) ||
			   (ptype == SCTP_SUCCESS_REPORT)) {
			 ;
		} else {
			if ((ptype & 0x8000) == 0x0000) {
				






				break;
			}
		}

	next_param:
		offset += SCTP_SIZE32(plen);
		if (offset >= limit) {
			break;
		}
		phdr = sctp_get_next_param(m, offset, &parm_buf,
					   sizeof(parm_buf));
	}
	
	TAILQ_FOREACH_SAFE(net, &stcb->asoc.nets, sctp_next, nnet) {
		if ((net->dest_state & SCTP_ADDR_NOT_IN_ASSOC) ==
		    SCTP_ADDR_NOT_IN_ASSOC) {
			
			
			stcb->asoc.numnets--;
			TAILQ_REMOVE(&stcb->asoc.nets, net, sctp_next);
			sctp_free_remote_addr(net);
			if (net == stcb->asoc.primary_destination) {
				stcb->asoc.primary_destination = NULL;
				sctp_select_primary_destination(stcb);
			}
		}
	}
	if (ecn_allowed == 0) {
		stcb->asoc.ecn_allowed = 0;
	}
	
	if (got_random && got_hmacs) {
		stcb->asoc.peer_supports_auth = 1;
	} else {
		stcb->asoc.peer_supports_auth = 0;
	}
	if (!stcb->asoc.peer_supports_auth && got_chklist) {
		
		return (-31);
	}
	if (!SCTP_BASE_SYSCTL(sctp_asconf_auth_nochk) && stcb->asoc.peer_supports_asconf &&
	    !stcb->asoc.peer_supports_auth) {
		
		return (-32);
	} else if ((stcb->asoc.peer_supports_asconf) && (stcb->asoc.peer_supports_auth) &&
		   ((saw_asconf == 0) || (saw_asconf_ack == 0))) {
		return (-33);
	}
	
	keylen = sizeof(*p_random) + random_len + sizeof(*hmacs) + hmacs_len;
	if (chunks != NULL) {
		keylen += sizeof(*chunks) + num_chunks;
	}
	new_key = sctp_alloc_key(keylen);
	if (new_key != NULL) {
		
		if (p_random != NULL) {
			keylen = sizeof(*p_random) + random_len;
			bcopy(p_random, new_key->key, keylen);
		}
		
		if (chunks != NULL) {
			bcopy(chunks, new_key->key + keylen,
			      sizeof(*chunks) + num_chunks);
			keylen += sizeof(*chunks) + num_chunks;
		}
		
		if (hmacs != NULL) {
			bcopy(hmacs, new_key->key + keylen,
			      sizeof(*hmacs) + hmacs_len);
		}
	} else {
		
		return (-34);
	}
	if (stcb->asoc.authinfo.peer_random != NULL)
		sctp_free_key(stcb->asoc.authinfo.peer_random);
	stcb->asoc.authinfo.peer_random = new_key;
	sctp_clear_cachedkeys(stcb, stcb->asoc.authinfo.assoc_keyid);
	sctp_clear_cachedkeys(stcb, stcb->asoc.authinfo.recv_keyid);

	return (0);
}

int
sctp_set_primary_addr(struct sctp_tcb *stcb, struct sockaddr *sa,
		      struct sctp_nets *net)
{
	
	if (net == NULL && sa)
		net = sctp_findnet(stcb, sa);

	if (net == NULL) {
		
		return (-1);
	} else {
		
		if (net->dest_state & SCTP_ADDR_UNCONFIRMED) {
			
			net->dest_state |= SCTP_ADDR_REQ_PRIMARY;
			return (0);
		}
		stcb->asoc.primary_destination = net;
		if (!(net->dest_state & SCTP_ADDR_PF) && (stcb->asoc.alternate)) {
			sctp_free_remote_addr(stcb->asoc.alternate);
			stcb->asoc.alternate = NULL;
		}
		net = TAILQ_FIRST(&stcb->asoc.nets);
		if (net != stcb->asoc.primary_destination) {
			




			TAILQ_REMOVE(&stcb->asoc.nets, stcb->asoc.primary_destination, sctp_next);
			TAILQ_INSERT_HEAD(&stcb->asoc.nets, stcb->asoc.primary_destination, sctp_next);
		}
		return (0);
	}
}

int
sctp_is_vtag_good(uint32_t tag, uint16_t lport, uint16_t rport, struct timeval *now)
{
	





	struct sctpvtaghead *chain;
	struct sctp_tagblock *twait_block;
	struct sctpasochead *head;
	struct sctp_tcb *stcb;
	int i;

	SCTP_INP_INFO_RLOCK();
	head = &SCTP_BASE_INFO(sctp_asochash)[SCTP_PCBHASH_ASOC(tag,
								SCTP_BASE_INFO(hashasocmark))];
	if (head == NULL) {
		
		goto skip_vtag_check;
	}
	LIST_FOREACH(stcb, head, sctp_asocs) {
		





		if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE) {
			continue;
		}
		if (stcb->asoc.my_vtag == tag) {
			
			if (stcb->rport != rport) {
				continue;
			}
			if (stcb->sctp_ep->sctp_lport != lport) {
				continue;
			}
			
			SCTP_INP_INFO_RUNLOCK();
			return (0);
		}
	}
skip_vtag_check:

	chain = &SCTP_BASE_INFO(vtag_timewait)[(tag % SCTP_STACK_VTAG_HASH_SIZE)];
	
	if (!LIST_EMPTY(chain)) {
		



		LIST_FOREACH(twait_block, chain, sctp_nxt_tagblock) {
			for (i = 0; i < SCTP_NUMBER_IN_VTAG_BLOCK; i++) {
				if (twait_block->vtag_block[i].v_tag == 0) {
					
					continue;
				} else if ((long)twait_block->vtag_block[i].tv_sec_at_expire  <
					   now->tv_sec) {
					
					twait_block->vtag_block[i].tv_sec_at_expire = 0;
					twait_block->vtag_block[i].v_tag = 0;
					twait_block->vtag_block[i].lport = 0;
					twait_block->vtag_block[i].rport = 0;
				} else if ((twait_block->vtag_block[i].v_tag == tag) &&
					   (twait_block->vtag_block[i].lport == lport) &&
					   (twait_block->vtag_block[i].rport == rport)) {
					
					SCTP_INP_INFO_RUNLOCK();
					return (0);
				}
			}
		}
	}
	SCTP_INP_INFO_RUNLOCK();
	return (1);
}

static void
sctp_drain_mbufs(struct sctp_tcb *stcb)
{
	



	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk, *nchk;
	uint32_t cumulative_tsn_p1;
	struct sctp_queued_to_read *ctl, *nctl;
	int cnt, strmat;
	uint32_t gap, i;
	int fnd = 0;

	

	asoc = &stcb->asoc;
	if (asoc->cumulative_tsn == asoc->highest_tsn_inside_map) {
		
		return;
	}
	SCTP_STAT_INCR(sctps_protocol_drains_done);
	cumulative_tsn_p1 = asoc->cumulative_tsn + 1;
	cnt = 0;
	
	TAILQ_FOREACH_SAFE(chk, &asoc->reasmqueue, sctp_next, nchk) {
		if (SCTP_TSN_GT(chk->rec.data.TSN_seq, cumulative_tsn_p1)) {
			
			cnt++;
			SCTP_CALC_TSN_TO_GAP(gap, chk->rec.data.TSN_seq, asoc->mapping_array_base_tsn);
			asoc->size_on_reasm_queue = sctp_sbspace_sub(asoc->size_on_reasm_queue, chk->send_size);
			sctp_ucount_decr(asoc->cnt_on_reasm_queue);
			SCTP_UNSET_TSN_PRESENT(asoc->mapping_array, gap);
			TAILQ_REMOVE(&asoc->reasmqueue, chk, sctp_next);
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		}
	}
	
	for (strmat = 0; strmat < asoc->streamincnt; strmat++) {
		TAILQ_FOREACH_SAFE(ctl, &asoc->strmin[strmat].inqueue, next, nctl) {
			if (SCTP_TSN_GT(ctl->sinfo_tsn, cumulative_tsn_p1)) {
				
				cnt++;
				SCTP_CALC_TSN_TO_GAP(gap, ctl->sinfo_tsn, asoc->mapping_array_base_tsn);
				asoc->size_on_all_streams = sctp_sbspace_sub(asoc->size_on_all_streams, ctl->length);
				sctp_ucount_decr(asoc->cnt_on_all_streams);
				SCTP_UNSET_TSN_PRESENT(asoc->mapping_array, gap);
				TAILQ_REMOVE(&asoc->strmin[strmat].inqueue, ctl, next);
				if (ctl->data) {
					sctp_m_freem(ctl->data);
					ctl->data = NULL;
				}
				sctp_free_remote_addr(ctl->whoFrom);
				SCTP_ZONE_FREE(SCTP_BASE_INFO(ipi_zone_readq), ctl);
				SCTP_DECR_READQ_COUNT();
			}
		}
	}
	if (cnt) {
		
		for (i = asoc->highest_tsn_inside_map; SCTP_TSN_GE(i, asoc->mapping_array_base_tsn); i--) {
			SCTP_CALC_TSN_TO_GAP(gap, i, asoc->mapping_array_base_tsn);
			if (SCTP_IS_TSN_PRESENT(asoc->mapping_array, gap)) {
				asoc->highest_tsn_inside_map = i;
				fnd = 1;
				break;
			}
		}
		if (!fnd) {
			asoc->highest_tsn_inside_map = asoc->mapping_array_base_tsn - 1;
		}

		













#ifdef SCTP_DEBUG
		SCTPDBG(SCTP_DEBUG_PCB1, "Freed %d chunks from reneg harvest\n", cnt);
#endif
		



		asoc->last_revoke_count = cnt;
		(void)SCTP_OS_TIMER_STOP(&stcb->asoc.dack_timer.timer);
		
		sctp_send_sack(stcb, SCTP_SO_NOT_LOCKED);
		sctp_chunk_output(stcb->sctp_ep, stcb, SCTP_OUTPUT_FROM_DRAIN, SCTP_SO_NOT_LOCKED);
	}
	








}

void
sctp_drain()
{
	




#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
	VNET_ITERATOR_DECL(vnet_iter);
#else
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb;

	SCTP_STAT_INCR(sctps_protocol_drain_calls);
	if (SCTP_BASE_SYSCTL(sctp_do_drain) == 0) {
		return;
	}
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
	VNET_LIST_RLOCK_NOSLEEP();
	VNET_FOREACH(vnet_iter) {
		CURVNET_SET(vnet_iter);
		struct sctp_inpcb *inp;
		struct sctp_tcb *stcb;
#endif

#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
		SCTP_STAT_INCR(sctps_protocol_drain_calls);
		if (SCTP_BASE_SYSCTL(sctp_do_drain) == 0) {
#ifdef VIMAGE
			continue;
#else
			return;
#endif
		}
#endif
		SCTP_INP_INFO_RLOCK();
		LIST_FOREACH(inp, &SCTP_BASE_INFO(listhead), sctp_list) {
			
			SCTP_INP_RLOCK(inp);
			LIST_FOREACH(stcb, &inp->sctp_asoc_list, sctp_tcblist) {
				
				SCTP_TCB_LOCK(stcb);
				sctp_drain_mbufs(stcb);
				SCTP_TCB_UNLOCK(stcb);
			}
			SCTP_INP_RUNLOCK(inp);
		}
		SCTP_INP_INFO_RUNLOCK();
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
		CURVNET_RESTORE();
	}
	VNET_LIST_RUNLOCK_NOSLEEP();
#endif
}










int
sctp_initiate_iterator(inp_func inpf,
		       asoc_func af,
		       inp_func inpe,
		       uint32_t pcb_state,
		       uint32_t pcb_features,
		       uint32_t asoc_state,
		       void *argp,
		       uint32_t argi,
		       end_func ef,
		       struct sctp_inpcb *s_inp,
		       uint8_t chunk_output_off)
{
	struct sctp_iterator *it = NULL;

	if (af == NULL) {
		return (-1);
	}
	SCTP_MALLOC(it, struct sctp_iterator *, sizeof(struct sctp_iterator),
		    SCTP_M_ITER);
	if (it == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_PCB, ENOMEM);
		return (ENOMEM);
	}
	memset(it, 0, sizeof(*it));
	it->function_assoc = af;
	it->function_inp = inpf;
	if (inpf)
		it->done_current_ep = 0;
	else
		it->done_current_ep = 1;
	it->function_atend = ef;
	it->pointer = argp;
	it->val = argi;
	it->pcb_flags = pcb_state;
	it->pcb_features = pcb_features;
	it->asoc_state = asoc_state;
	it->function_inp_end = inpe;
	it->no_chunk_output = chunk_output_off;
#if defined(__FreeBSD__) && __FreeBSD_version >= 801000
	it->vn = curvnet;
#endif
	if (s_inp) {
		
		it->inp = s_inp;
		SCTP_INP_INCR_REF(it->inp);
		it->iterator_flags = SCTP_ITERATOR_DO_SINGLE_INP;
	} else {
		SCTP_INP_INFO_RLOCK();
		it->inp = LIST_FIRST(&SCTP_BASE_INFO(listhead));
		if (it->inp) {
			SCTP_INP_INCR_REF(it->inp);
		}
		SCTP_INP_INFO_RUNLOCK();
		it->iterator_flags = SCTP_ITERATOR_DO_ALL_INP;

	}
	SCTP_IPI_ITERATOR_WQ_LOCK();

	TAILQ_INSERT_TAIL(&sctp_it_ctl.iteratorhead, it, sctp_nxt_itr);
	if (sctp_it_ctl.iterator_running == 0) {
		sctp_wakeup_iterator();
	}
	SCTP_IPI_ITERATOR_WQ_UNLOCK();
	
	return (0);
}
