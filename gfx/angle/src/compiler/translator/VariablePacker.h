





#ifndef _VARIABLEPACKER_INCLUDED_
#define _VARIABLEPACKER_INCLUDED_

#include <vector>
#include "compiler/translator/VariableInfo.h"

class VariablePacker {
 public:
    
    
    template <typename VarT>
    bool CheckVariablesWithinPackingLimits(unsigned int maxVectors,
                                           const std::vector<VarT> &in_variables);

    
    static int GetNumComponentsPerRow(sh::GLenum type);

    
    static int GetNumRows(sh::GLenum type);

  private:
    static const int kNumColumns = 4;
    static const unsigned kColumnMask = (1 << kNumColumns) - 1;

    unsigned makeColumnFlags(int column, int numComponentsPerRow);
    void fillColumns(int topRow, int numRows, int column, int numComponentsPerRow);
    bool searchColumn(int column, int numRows, int* destRow, int* destSize);

    int topNonFullRow_;
    int bottomNonFullRow_;
    int maxRows_;
    std::vector<unsigned> rows_;
};

#endif 
