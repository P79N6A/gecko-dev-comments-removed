









#include "LETypes.h"
#include "ThaiShaping.h"

U_NAMESPACE_BEGIN

const le_uint8 ThaiShaping::classTable[] = {
    
    
     NON, CON, CON, CON, CON, CON, CON, CON, CON, CON, CON, CON, CON, COD, COD, COD, 
     COD, CON, CON, CON, CON, CON, CON, CON, CON, CON, CON, COA, CON, COA, CON, COA, 
     CON, CON, CON, CON, FV3, CON, FV3, CON, CON, CON, CON, CON, CON, CON, CON, NON, 
     FV1, AV2, FV1, FV1, AV1, AV3, AV2, AV3, BV1, BV2, BDI, NON, NON, NON, NON, NON, 
     LVO, LVO, LVO, LVO, LVO, FV2, NON, AD2, TON, TON, TON, TON, AD1, NIK, AD3, NON, 
     NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON
};

const ThaiShaping::StateTransition ThaiShaping::thaiStateTable[][ThaiShaping::classCount] = {
    
    
    
    
    
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 2, tC}, { 6, tC}, { 0, tC}, { 8, tE}, { 0, tE}, { 0, tE}, { 0, tC}, { 9, tE}, {11, tC}, {14, tC}, {16, tC}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 3, tE}, { 0, tE}, { 0, tR}, { 0, tR}, { 4, tE}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 5, tC}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 7, tE}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {10, tC}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {12, tC}, { 0, tC}, { 0, tR}, { 0, tR}, {13, tC}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {15, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {17, tC}, { 0, tR}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tS}, { 0, tA}, {19, tC}, {23, tC}, { 0, tC}, {25, tF}, { 0, tF}, { 0, tF}, { 0, tD}, {26, tF}, {28, tD}, {31, tD}, {33, tD}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {20, tF}, { 0, tF}, { 0, tR}, { 0, tR}, {21, tF}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {22, tC}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {24, tF}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {27, tG}, { 0, tG}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {29, tG}, { 0, tG}, { 0, tR}, { 0, tR}, {30, tG}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {32, tG}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {34, tG}, { 0, tR}, { 0, tG}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tS}, { 0, tA}, {36, tH}, {40, tH}, { 0, tH}, {42, tE}, { 0, tE}, { 0, tE}, { 0, tC}, {43, tE}, {45, tC}, {48, tC}, {50, tC}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {37, tE}, { 0, tE}, { 0, tR}, { 0, tR}, {38, tE}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {39, tC}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {41, tE}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {44, tC}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {46, tC}, { 0, tC}, { 0, tR}, { 0, tR}, {47, tC}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {49, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tS}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, {51, tC}, { 0, tR}, { 0, tC}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}},
     {{ 0, tA}, { 1, tA}, {18, tA}, {35, tA}, { 0, tA}, { 0, tS}, { 0, tA}, { 0, tA}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}, { 0, tR}}
};

U_NAMESPACE_END
