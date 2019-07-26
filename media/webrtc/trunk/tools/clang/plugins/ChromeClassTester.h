



#ifndef TOOLS_CLANG_PLUGINS_CHROMECLASSTESTER_H_
#define TOOLS_CLANG_PLUGINS_CHROMECLASSTESTER_H_

#include <set>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Frontend/CompilerInstance.h"



class ChromeClassTester : public clang::ASTConsumer {
 public:
  explicit ChromeClassTester(clang::CompilerInstance& instance);
  virtual ~ChromeClassTester();

  
  virtual void HandleTagDeclDefinition(clang::TagDecl* tag);
  virtual bool HandleTopLevelDecl(clang::DeclGroupRef group_ref);

 protected:
  clang::CompilerInstance& instance() { return instance_; }
  clang::DiagnosticsEngine& diagnostic() { return diagnostic_; }

  
  
  void emitWarning(clang::SourceLocation loc, const char* error);

  
  
  bool InBannedNamespace(const clang::Decl* record);

  
  
  
  std::string GetNamespace(const clang::Decl* record);

  
  
  bool InImplementationFile(clang::SourceLocation location);

 private:
  void BuildBannedLists();

  void CheckTag(clang::TagDecl*);

  
  
  virtual void CheckChromeClass(clang::SourceLocation record_location,
                                clang::CXXRecordDecl* record) = 0;

  
  
  std::string GetNamespaceImpl(const clang::DeclContext* context,
                               const std::string& candidate);
  bool InBannedDirectory(clang::SourceLocation loc);
  bool IsIgnoredType(const std::string& base_name);

  
  
  bool GetFilename(clang::SourceLocation loc, std::string* filename);

  clang::CompilerInstance& instance_;
  clang::DiagnosticsEngine& diagnostic_;

  
  std::vector<std::string> banned_namespaces_;

  
  std::vector<std::string> banned_directories_;

  
  std::set<std::string> ignored_record_names_;

  
  std::vector<clang::TagDecl*> pending_class_decls_;
};

#endif  
