





#ifndef _INITIALIZE_INCLUDED_
#define _INITIALIZE_INCLUDED_

#include "compiler/translator/Common.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/SymbolTable.h"

void InsertBuiltInFunctions(sh::GLenum type, ShShaderSpec spec, const ShBuiltInResources &resources, TSymbolTable &table);

void IdentifyBuiltIns(sh::GLenum type, ShShaderSpec spec,
                      const ShBuiltInResources& resources,
                      TSymbolTable& symbolTable);

void InitExtensionBehavior(const ShBuiltInResources& resources,
                           TExtensionBehavior& extensionBehavior);

#endif 
