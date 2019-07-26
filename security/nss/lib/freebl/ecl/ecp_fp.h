



#ifndef __ecp_fp_h_
#define __ecp_fp_h_

#include "mpi.h"
#include "ecl.h"
#include "ecp.h"

#include <sys/types.h>
#include "mpi-priv.h"

#ifdef ECL_DEBUG
#include <assert.h>
#endif



#define ECFP_MAXDOUBLES 10


#ifndef ECL_DEBUG
#define ECFP_ASSERT(x)
#else
#define ECFP_ASSERT(x) assert(x)
#endif



#define ECFP_T0 1.0
#define ECFP_T1 16777216.0
#define ECFP_T2 281474976710656.0
#define ECFP_T3 4722366482869645213696.0
#define ECFP_T4 79228162514264337593543950336.0
#define ECFP_T5 1329227995784915872903807060280344576.0
#define ECFP_T6 22300745198530623141535718272648361505980416.0
#define ECFP_T7 374144419156711147060143317175368453031918731001856.0
#define ECFP_T8 6277101735386680763835789423207666416102355444464034512896.0
#define ECFP_T9 105312291668557186697918027683670432318895095400549111254310977536.0
#define ECFP_T10  1766847064778384329583297500742918515827483896875618958121606201292619776.0
#define ECFP_T11 29642774844752946028434172162224104410437116074403984394101141506025761187823616.0
#define ECFP_T12 497323236409786642155382248146820840100456150797347717440463976893159497012533375533056.0
#define ECFP_T13  8343699359066055009355553539724812947666814540455674882605631280555545803830627148527195652096.0
#define ECFP_T14 139984046386112763159840142535527767382602843577165595931249318810236991948760059086304843329475444736.0
#define ECFP_T15 2348542582773833227889480596789337027375682548908319870707290971532209025114608443463698998384768703031934976.0
#define ECFP_T16 39402006196394479212279040100143613805079739270465446667948293404245\
721771497210611414266254884915640806627990306816.0
#define ECFP_T17 66105596879024859895191530803277103982840468296428121928464879527440\
5791236311345825189210439715284847591212025023358304256.0
#define ECFP_T18 11090678776483259438313656736572334813745748301503266300681918322458\
485231222502492159897624416558312389564843845614287315896631296.0
#define ECFP_T19 18607071341967536398062689481932916079453218833595342343206149099024\
36577570298683715049089827234727835552055312041415509848580169253519\
36.0

#define ECFP_TWO160 1461501637330902918203684832716283019655932542976.0
#define ECFP_TWO192 6277101735386680763835789423207666416102355444464034512896.0
#define ECFP_TWO224 26959946667150639794667015087019630673637144422540572481103610249216.0


static const double ecfp_two32 = 4294967296.0;
static const double ecfp_two64 = 18446744073709551616.0;
static const double ecfp_twom16 = .0000152587890625;
static const double ecfp_twom128 =
	.00000000000000000000000000000000000000293873587705571876992184134305561419454666389193021880377187926569604314863681793212890625;
static const double ecfp_twom129 =
	.000000000000000000000000000000000000001469367938527859384960920671527807097273331945965109401885939632848021574318408966064453125;
static const double ecfp_twom160 =
	.0000000000000000000000000000000000000000000000006842277657836020854119773355907793609766904013068924666782559979930620520927053718196475529111921787261962890625;
static const double ecfp_twom192 =
	.000000000000000000000000000000000000000000000000000000000159309191113245227702888039776771180559110455519261878607388585338616290151305816094308987472018268594098344692611135542392730712890625;
static const double ecfp_twom224 =
	.00000000000000000000000000000000000000000000000000000000000000000003709206150687421385731735261547639513367564778757791002453039058917581340095629358997312082723208437536338919136001159027049567384892725385725498199462890625;


static const double ecfp_exp[2 * ECFP_MAXDOUBLES] = {
	ECFP_T0, ECFP_T1, ECFP_T2, ECFP_T3, ECFP_T4, ECFP_T5,
	ECFP_T6, ECFP_T7, ECFP_T8, ECFP_T9, ECFP_T10, ECFP_T11,
	ECFP_T12, ECFP_T13, ECFP_T14, ECFP_T15, ECFP_T16, ECFP_T17, ECFP_T18,
	ECFP_T19
};




#define ECFP_ALPHABASE_53 6755399441055744.0




#define ECFP_ALPHABASE_64 13835058055282163712.0





static const double ecfp_alpha_53[2 * ECFP_MAXDOUBLES] = {
	ECFP_ALPHABASE_53 * ECFP_T0,
	ECFP_ALPHABASE_53 * ECFP_T1,
	ECFP_ALPHABASE_53 * ECFP_T2,
	ECFP_ALPHABASE_53 * ECFP_T3,
	ECFP_ALPHABASE_53 * ECFP_T4,
	ECFP_ALPHABASE_53 * ECFP_T5,
	ECFP_ALPHABASE_53 * ECFP_T6,
	ECFP_ALPHABASE_53 * ECFP_T7,
	ECFP_ALPHABASE_53 * ECFP_T8,
	ECFP_ALPHABASE_53 * ECFP_T9,
	ECFP_ALPHABASE_53 * ECFP_T10,
	ECFP_ALPHABASE_53 * ECFP_T11,
	ECFP_ALPHABASE_53 * ECFP_T12,
	ECFP_ALPHABASE_53 * ECFP_T13,
	ECFP_ALPHABASE_53 * ECFP_T14,
	ECFP_ALPHABASE_53 * ECFP_T15,
	ECFP_ALPHABASE_53 * ECFP_T16,
	ECFP_ALPHABASE_53 * ECFP_T17,
	ECFP_ALPHABASE_53 * ECFP_T18,
	ECFP_ALPHABASE_53 * ECFP_T19
};





static const double ecfp_alpha_64[2 * ECFP_MAXDOUBLES] = {
	ECFP_ALPHABASE_64 * ECFP_T0,
	ECFP_ALPHABASE_64 * ECFP_T1,
	ECFP_ALPHABASE_64 * ECFP_T2,
	ECFP_ALPHABASE_64 * ECFP_T3,
	ECFP_ALPHABASE_64 * ECFP_T4,
	ECFP_ALPHABASE_64 * ECFP_T5,
	ECFP_ALPHABASE_64 * ECFP_T6,
	ECFP_ALPHABASE_64 * ECFP_T7,
	ECFP_ALPHABASE_64 * ECFP_T8,
	ECFP_ALPHABASE_64 * ECFP_T9,
	ECFP_ALPHABASE_64 * ECFP_T10,
	ECFP_ALPHABASE_64 * ECFP_T11,
	ECFP_ALPHABASE_64 * ECFP_T12,
	ECFP_ALPHABASE_64 * ECFP_T13,
	ECFP_ALPHABASE_64 * ECFP_T14,
	ECFP_ALPHABASE_64 * ECFP_T15,
	ECFP_ALPHABASE_64 * ECFP_T16,
	ECFP_ALPHABASE_64 * ECFP_T17,
	ECFP_ALPHABASE_64 * ECFP_T18,
	ECFP_ALPHABASE_64 * ECFP_T19
};


#define ECFP_BETABASE 0.4999999701976776123046875





static const double ecfp_beta[2 * ECFP_MAXDOUBLES] = {
	ECFP_BETABASE * ECFP_T0,
	ECFP_BETABASE * ECFP_T1,
	ECFP_BETABASE * ECFP_T2,
	ECFP_BETABASE * ECFP_T3,
	ECFP_BETABASE * ECFP_T4,
	ECFP_BETABASE * ECFP_T5,
	ECFP_BETABASE * ECFP_T6,
	ECFP_BETABASE * ECFP_T7,
	ECFP_BETABASE * ECFP_T8,
	ECFP_BETABASE * ECFP_T9,
	ECFP_BETABASE * ECFP_T10,
	ECFP_BETABASE * ECFP_T11,
	ECFP_BETABASE * ECFP_T12,
	ECFP_BETABASE * ECFP_T13,
	ECFP_BETABASE * ECFP_T14,
	ECFP_BETABASE * ECFP_T15,
	ECFP_BETABASE * ECFP_T16,
	ECFP_BETABASE * ECFP_T17,
	ECFP_BETABASE * ECFP_T18,
	ECFP_BETABASE * ECFP_T19
};

static const double ecfp_beta_160 = ECFP_BETABASE * ECFP_TWO160;
static const double ecfp_beta_192 = ECFP_BETABASE * ECFP_TWO192;
static const double ecfp_beta_224 = ECFP_BETABASE * ECFP_TWO224;



typedef struct {
	double x[ECFP_MAXDOUBLES];
	double y[ECFP_MAXDOUBLES];
} ecfp_aff_pt;




typedef struct {
	double x[ECFP_MAXDOUBLES];
	double y[ECFP_MAXDOUBLES];
	double z[ECFP_MAXDOUBLES];
} ecfp_jac_pt;



typedef struct {
	double x[ECFP_MAXDOUBLES];
	double y[ECFP_MAXDOUBLES];
	double z[ECFP_MAXDOUBLES];
	double z2[ECFP_MAXDOUBLES];
	double z3[ECFP_MAXDOUBLES];
} ecfp_chud_pt;



typedef struct {
	double x[ECFP_MAXDOUBLES];
	double y[ECFP_MAXDOUBLES];
	double z[ECFP_MAXDOUBLES];
	double az4[ECFP_MAXDOUBLES];
} ecfp_jm_pt;

struct EC_group_fp_str;

typedef struct EC_group_fp_str EC_group_fp;
struct EC_group_fp_str {
	int fpPrecision;			

	int numDoubles;
	int primeBitSize;
	int orderBitSize;
	int doubleBitSize;
	int numInts;
	int aIsM3;					

	double curvea[ECFP_MAXDOUBLES];
	
	double bitSize_alpha;
	
	const double *alpha;

	void (*ecfp_singleReduce) (double *r, const EC_group_fp * group);
	void (*ecfp_reduce) (double *r, double *x, const EC_group_fp * group);
	



	void (*ecfp_tidy) (double *t, const double *alpha,
					   const EC_group_fp * group);
	


	void (*pt_add_jac_aff) (const ecfp_jac_pt * p, const ecfp_aff_pt * q,
							ecfp_jac_pt * r, const EC_group_fp * group);
	

	void (*pt_dbl_jac) (const ecfp_jac_pt * dp, ecfp_jac_pt * dr,
						const EC_group_fp * group);
	

	void (*pt_add_jac) (const ecfp_jac_pt * p, const ecfp_jac_pt * q,
						ecfp_jac_pt * r, const EC_group_fp * group);
	

	void (*pt_dbl_jm) (const ecfp_jm_pt * p, ecfp_jm_pt * r,
					   const EC_group_fp * group);
	


	void (*pt_dbl_aff2chud) (const ecfp_aff_pt * p, ecfp_chud_pt * r,
							 const EC_group_fp * group);
	


	void (*pt_add_jm_chud) (ecfp_jm_pt * p, ecfp_chud_pt * q,
							ecfp_jm_pt * r, const EC_group_fp * group);
	


	void (*pt_add_chud) (const ecfp_chud_pt * p, const ecfp_chud_pt * q,
						 ecfp_chud_pt * r, const EC_group_fp * group);
	



	void (*precompute_chud) (ecfp_chud_pt * out, const ecfp_aff_pt * p,
							 const EC_group_fp * group);
	

	void (*precompute_jac) (ecfp_jac_pt * out, const ecfp_aff_pt * p,
							const EC_group_fp * group);

};




void ecfp_multiply(double *r, const double *x, const double *y);





void ecfp_tidy(double *t, const double *alpha, const EC_group_fp * group);




void ecfp_tidyUpper(double *t, const EC_group_fp * group);



void ecfp_tidyShort(double *t, const EC_group_fp * group);




void ecfp_positiveTidy(double *t, const EC_group_fp * group);






mp_err
 ec_GFp_point_mul_jac_4w_fp(const mp_int *n, const mp_int *px,
							const mp_int *py, mp_int *rx, mp_int *ry,
							const ECGroup *ecgroup);










mp_err ec_GFp_point_mul_wNAF_fp(const mp_int *n, const mp_int *px,
								const mp_int *py, mp_int *rx, mp_int *ry,
								const ECGroup *ecgroup);













mp_err
 ec_GFp_pt_mul_jac_fp(const mp_int *n, const mp_int *px, const mp_int *py,
					  mp_int *rx, mp_int *ry, const ECGroup *ecgroup);


void ec_GFp_extra_free_fp(ECGroup *group);



void
 ecfp_fp2i(mp_int *mpout, double *d, const ECGroup *ecgroup);


void
 ecfp_i2fp(double *out, const mp_int *x, const ECGroup *ecgroup);





int ec_set_fp_precision(EC_group_fp * group);

#endif
