






#include "ChromeClassTester.h"

#include <sys/param.h>

#include "clang/AST/AST.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

namespace {

bool starts_with(const std::string& one, const std::string& two) {
  return one.compare(0, two.size(), two) == 0;
}

std::string lstrip(const std::string& one, const std::string& two) {
  if (starts_with(one, two))
    return one.substr(two.size());
  return one;
}

bool ends_with(const std::string& one, const std::string& two) {
  if (two.size() > one.size())
    return false;

  return one.compare(one.size() - two.size(), two.size(), two) == 0;
}

}  

ChromeClassTester::ChromeClassTester(CompilerInstance& instance)
    : instance_(instance),
      diagnostic_(instance.getDiagnostics()) {
  BuildBannedLists();
}

ChromeClassTester::~ChromeClassTester() {}

void ChromeClassTester::HandleTagDeclDefinition(TagDecl* tag) {
  pending_class_decls_.push_back(tag);
}

bool ChromeClassTester::HandleTopLevelDecl(DeclGroupRef group_ref) {
  for (size_t i = 0; i < pending_class_decls_.size(); ++i)
    CheckTag(pending_class_decls_[i]);
  pending_class_decls_.clear();

  return true;  
}

void ChromeClassTester::CheckTag(TagDecl* tag) {
  
  
  

  if (CXXRecordDecl* record = dyn_cast<CXXRecordDecl>(tag)) {
    
    
    
    if (record->isPOD() ||
        record->getDescribedClassTemplate() ||
        record->getTemplateSpecializationKind() ||
        record->isDependentType())
      return;

    if (InBannedNamespace(record))
      return;

    SourceLocation record_location = record->getInnerLocStart();
    if (InBannedDirectory(record_location))
      return;

    
    
    
    std::string base_name = record->getNameAsString();
    if (IsIgnoredType(base_name))
      return;

    
    
    if (ends_with(base_name, "Matcher"))
        return;

    CheckChromeClass(record_location, record);
  }
}

void ChromeClassTester::emitWarning(SourceLocation loc,
                                    const char* raw_error) {
  FullSourceLoc full(loc, instance().getSourceManager());
  std::string err;
  err = "[chromium-style] ";
  err += raw_error;
  DiagnosticsEngine::Level level =
      diagnostic().getWarningsAsErrors() ?
      DiagnosticsEngine::Error :
      DiagnosticsEngine::Warning;
  unsigned id = diagnostic().getCustomDiagID(level, err);
  DiagnosticBuilder builder = diagnostic().Report(full, id);
}

bool ChromeClassTester::InBannedNamespace(const Decl* record) {
  std::string n = GetNamespace(record);
  if (!n.empty()) {
    return std::find(banned_namespaces_.begin(), banned_namespaces_.end(), n)
        != banned_namespaces_.end();
  }

  return false;
}

std::string ChromeClassTester::GetNamespace(const Decl* record) {
  return GetNamespaceImpl(record->getDeclContext(), "");
}

bool ChromeClassTester::InImplementationFile(SourceLocation record_location) {
  std::string filename;
  if (!GetFilename(record_location, &filename))
    return false;

  if (ends_with(filename, ".cc") || ends_with(filename, ".cpp") ||
      ends_with(filename, ".mm")) {
    return true;
  }

  return false;
}

void ChromeClassTester::BuildBannedLists() {
  banned_namespaces_.push_back("std");
  banned_namespaces_.push_back("__gnu_cxx");
  banned_namespaces_.push_back("WebKit");

  banned_directories_.push_back("third_party/");
  banned_directories_.push_back("native_client/");
  banned_directories_.push_back("breakpad/");
  banned_directories_.push_back("courgette/");
  banned_directories_.push_back("pdf/");
  banned_directories_.push_back("ppapi/");
  banned_directories_.push_back("usr/");
  banned_directories_.push_back("testing/");
  banned_directories_.push_back("googleurl/");
  banned_directories_.push_back("v8/");
  banned_directories_.push_back("dart/");
  banned_directories_.push_back("sdch/");
  banned_directories_.push_back("icu4c/");
  banned_directories_.push_back("frameworks/");

  
  
  
  
  banned_directories_.push_back("gen/");
  banned_directories_.push_back("geni/");
  banned_directories_.push_back("xcodebuild/");

  
  
  banned_directories_.push_back("automation/");

  
  banned_directories_.push_back("/Developer/");

  
  
  ignored_record_names_.insert("ThreadLocalBoolean");

  
  ignored_record_names_.insert("Header");

  
  
  ignored_record_names_.insert("Validators");

  
  ignored_record_names_.insert("AutocompleteController");
  ignored_record_names_.insert("HistoryURLProvider");

  
  ignored_record_names_.insert("ReliabilityTestSuite");

  
  
  ignored_record_names_.insert("MockTransaction");

  
  
  ignored_record_names_.insert("TestAnimationDelegate");

  
  
  ignored_record_names_.insert("PluginVersionInfo");
}

std::string ChromeClassTester::GetNamespaceImpl(const DeclContext* context,
                                                const std::string& candidate) {
  switch (context->getDeclKind()) {
    case Decl::TranslationUnit: {
      return candidate;
    }
    case Decl::Namespace: {
      const NamespaceDecl* decl = dyn_cast<NamespaceDecl>(context);
      std::string name_str;
      llvm::raw_string_ostream OS(name_str);
      if (decl->isAnonymousNamespace())
        OS << "<anonymous namespace>";
      else
        OS << *decl;
      return GetNamespaceImpl(context->getParent(),
                              OS.str());
    }
    default: {
      return GetNamespaceImpl(context->getParent(), candidate);
    }
  }
}

bool ChromeClassTester::InBannedDirectory(SourceLocation loc) {
  std::string filename;
  if (!GetFilename(loc, &filename)) {
    
    
    return true;
  }

  
  
  
  if (filename == "<scratch space>")
    return true;

  
  if (ends_with(filename, ".pb.h")) {
    return true;
  }

  
  
  
  char resolvedPath[MAXPATHLEN];
  if (realpath(filename.c_str(), resolvedPath)) {
    filename = resolvedPath;
  }

  
  
  
  filename = lstrip(filename, "/usr/local/google");

  for (std::vector<std::string>::const_iterator it =
           banned_directories_.begin();
       it != banned_directories_.end(); ++it) {
    
    
    size_t index = filename.find(*it);
    if (index != std::string::npos) {
      bool matches_full_dir_name = index == 0 || filename[index - 1] == '/';
      if ((*it)[0] == '/')
        matches_full_dir_name = true;
      if (matches_full_dir_name)
        return true;
    }
  }

  return false;
}

bool ChromeClassTester::IsIgnoredType(const std::string& base_name) {
  return ignored_record_names_.find(base_name) != ignored_record_names_.end();
}

bool ChromeClassTester::GetFilename(SourceLocation loc,
                                    std::string* filename) {
  const SourceManager& source_manager = instance_.getSourceManager();
  SourceLocation spelling_location = source_manager.getSpellingLoc(loc);
  PresumedLoc ploc = source_manager.getPresumedLoc(spelling_location);
  if (ploc.isInvalid()) {
    
    
    return false;
  }

  *filename = ploc.getFilename();
  return true;
}
