








































#define MILLISECONDS_TO_SAMPLES(PERIOD)   (short)((PERIOD) << 3)

#define TGN_COEF_300        (short)31863
#define TGN_COEF_350        (short)31538
#define TGN_COEF_425        (short)30959
#define TGN_COEF_440        (short)30831
#define TGN_COEF_450        (short)30743
#define TGN_COEF_480        (short)30467
#define TGN_COEF_500        (short)30274
#define TGN_COEF_548        (short)29780
#define TGN_COEF_600        (short)29197
#define TGN_COEF_620        (short)28959
#define TGN_COEF_697        (short)27980
#define TGN_COEF_770        (short)26956
#define TGN_COEF_852        (short)25701
#define TGN_COEF_941        (short)24219
#define TGN_COEF_1209       (short)19073
#define TGN_COEF_1336       (short)16325
#define TGN_COEF_1400       (short)14876
#define TGN_COEF_1477       (short)13085
#define TGN_COEF_1633       (short)9315
#define TGN_COEF_1000       (short)23170
#define TGN_COEF_1MW        (short)23098
#define TGN_COEF_1MW_neg15dBm   (short)23098

#define TGN_YN_2_300        (short)-840
#define TGN_YN_2_350        (short)-814
#define TGN_YN_2_425        (short)-1966
#define TGN_YN_2_440        (short)-2032
#define TGN_YN_2_450        (short)-1384
#define TGN_YN_2_480        (short)-1104
#define TGN_YN_2_500        (short)-1148
#define TGN_YN_2_548        (short)-1252
#define TGN_YN_2_600        (short)-2270
#define TGN_YN_2_620        (short)-1404
#define TGN_YN_2_697        (short)-1561
#define TGN_YN_2_770        (short)-1706
#define TGN_YN_2_852        (short)-1861
#define TGN_YN_2_941        (short)-2021
#define TGN_YN_2_1209       (short)-2439
#define TGN_YN_2_1336       (short)-2601
#define TGN_YN_2_1400       (short)-5346    //tone level=-11.61 dBm0, same as CallWaiting CSCsd65600
#define TGN_YN_2_1477       (short)-2750
#define TGN_YN_2_1633       (short)-2875
#define TGN_YN_2_1000       (short)-1414
#define TGN_YN_2_1MW        (short)-16192
#define TGN_YN_2_1MW_neg15dBm   (short)-2879


#define TGN_COEF_440_PREC_RB   (short)30831
#define TGN_COEF_480_PREC_RB   (short)30467
#define TGN_COEF_440_PREEMP    (short)30831
#define TGN_COEF_620_PREEMP    (short)28959
#define TGN_COEF_440_PREC_CW   (short)30831

#define TGN_YN_2_440_PREC_RB   (short)-1016
#define TGN_YN_2_480_PREC_RB   (short)-1104
#define TGN_YN_2_440_PREEMP    (short)-1016
#define TGN_YN_2_620_PREEMP    (short)-1404
#define TGN_YN_2_440_PREC_CW   (short)-1016
































































#define BEEP_REC_ON		MILLISECONDS_TO_SAMPLES(500)
#define BEEP_REC_OFF    MILLISECONDS_TO_SAMPLES((15000 - 500) / 2)
#define BEEP_MON_ON1	MILLISECONDS_TO_SAMPLES(1500)
#define BEEP_MON_OFF1   MILLISECONDS_TO_SAMPLES(8000)
#define BEEP_MON_ON2	MILLISECONDS_TO_SAMPLES(500)
#define BEEP_MON_OFF2   MILLISECONDS_TO_SAMPLES(8000)

