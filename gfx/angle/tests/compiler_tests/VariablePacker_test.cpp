




#include "compiler/VariablePacker.h"
#include "gtest/gtest.h"

TEST(VariablePacking, Pack) {
  VariablePacker packer;
  TVariableInfoList vars;
  const int kMaxRows = 16;
  
  EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

  ShDataType types[] = {
    SH_FLOAT_MAT4,            
    SH_FLOAT_MAT2,            
    SH_FLOAT_VEC4,            
    SH_INT_VEC4,              
    SH_BOOL_VEC4,             
    SH_FLOAT_MAT3,            
    SH_FLOAT_VEC3,            
    SH_INT_VEC3,              
    SH_BOOL_VEC3,             
    SH_FLOAT_VEC2,            
    SH_INT_VEC2,              
    SH_BOOL_VEC2,             
    SH_FLOAT,                 
    SH_INT,                   
    SH_BOOL,                  
    SH_SAMPLER_2D,            
    SH_SAMPLER_CUBE,          
    SH_SAMPLER_EXTERNAL_OES,  
    SH_SAMPLER_2D_RECT_ARB,   
  };

  for (size_t tt = 0; tt < sizeof(types) / sizeof(types[0]); ++tt) {
    ShDataType type = types[tt];
    int num_rows = VariablePacker::GetNumRows(type);
    int num_components_per_row = VariablePacker::GetNumComponentsPerRow(type);
    
    vars.clear();
    vars.push_back(TVariableInfo(type, 1));
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    
    int num_vars = kMaxRows / num_rows;
    vars.clear();
    vars.push_back(TVariableInfo(type, num_vars));
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    
    vars.clear();
    vars.push_back(TVariableInfo(type, num_vars + 1));
    EXPECT_FALSE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    
    num_vars = kMaxRows / num_rows *
        ((num_components_per_row > 2) ? 1 : (4 / num_components_per_row));
    vars.clear();
    for (int ii = 0; ii < num_vars; ++ii) {
      vars.push_back(TVariableInfo(type, 1));
    }
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    
    vars.push_back(TVariableInfo( type, 1));
    EXPECT_FALSE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));
  }

  
  vars.clear();
  vars.push_back(TVariableInfo(SH_FLOAT_VEC4, 1));
  vars.push_back(TVariableInfo(SH_FLOAT_MAT3, 1));
  vars.push_back(TVariableInfo(SH_FLOAT_MAT3, 1));
  vars.push_back(TVariableInfo(SH_FLOAT_VEC2, 6));
  vars.push_back(TVariableInfo(SH_FLOAT_VEC2, 4));
  vars.push_back(TVariableInfo(SH_FLOAT_VEC2, 1));
  vars.push_back(TVariableInfo(SH_FLOAT, 3));
  vars.push_back(TVariableInfo(SH_FLOAT, 2));
  vars.push_back(TVariableInfo(SH_FLOAT, 1));
  EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));
}

