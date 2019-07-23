









#ifndef BASE_PE_IMAGE_H_
#define BASE_PE_IMAGE_H_

#include <windows.h>
#include <DelayIMP.h>




class PEImage {
 public:
  
  
  
  typedef bool (*EnumSectionsFunction)(const PEImage &image,
                                       PIMAGE_SECTION_HEADER header,
                                       PVOID section_start, DWORD section_size,
                                       PVOID cookie);

  
  
  
  
  
  typedef bool (*EnumExportsFunction)(const PEImage &image, DWORD ordinal,
                                      DWORD hint, LPCSTR name, PVOID function,
                                      LPCSTR forward, PVOID cookie);

  
  
  
  
  typedef bool (*EnumImportChunksFunction)(const PEImage &image, LPCSTR module,
                                           PIMAGE_THUNK_DATA name_table,
                                           PIMAGE_THUNK_DATA iat, PVOID cookie);

  
  
  
  
  typedef bool (*EnumImportsFunction)(const PEImage &image, LPCSTR module,
                                      DWORD ordinal, LPCSTR name, DWORD hint,
                                      PIMAGE_THUNK_DATA iat, PVOID cookie);

  
  
  
  
  typedef bool (*EnumDelayImportChunksFunction)(const PEImage &image,
                                                PImgDelayDescr delay_descriptor,
                                                LPCSTR module,
                                                PIMAGE_THUNK_DATA name_table,
                                                PIMAGE_THUNK_DATA iat,
                                                PIMAGE_THUNK_DATA bound_iat,
                                                PIMAGE_THUNK_DATA unload_iat,
                                                PVOID cookie);

  
  
  
  typedef bool (*EnumRelocsFunction)(const PEImage &image, WORD type,
                                     PVOID address, PVOID cookie);

  explicit PEImage(HMODULE module) : module_(module) {}
  explicit PEImage(const void* module) {
    module_ = reinterpret_cast<HMODULE>(const_cast<void*>(module));
  }

  
  HMODULE module() const;

  
  void set_module(HMODULE module);

  
  static bool IsOrdinal(LPCSTR name);

  
  static WORD ToOrdinal(LPCSTR name);

  
  PIMAGE_DOS_HEADER GetDosHeader() const;

  
  PIMAGE_NT_HEADERS GetNTHeaders() const;

  
  WORD GetNumSections() const;

  
  
  PIMAGE_SECTION_HEADER GetSectionHeader(UINT section) const;

  
  DWORD GetImageDirectoryEntrySize(UINT directory) const;

  
  PVOID GetImageDirectoryEntryAddr(UINT directory) const;

  
  
  
  
  PIMAGE_SECTION_HEADER GetImageSectionFromAddr(PVOID address) const;

  
  PIMAGE_SECTION_HEADER GetImageSectionHeaderByName(LPCSTR section_name) const;

  
  PIMAGE_IMPORT_DESCRIPTOR GetFirstImportChunk() const;

  
  PIMAGE_EXPORT_DIRECTORY GetExportDirectory() const;

  
  
  
  
  
  
  PDWORD GetExportEntry(LPCSTR name) const;

  
  
  
  
  
  
  
  
  FARPROC GetProcAddress(LPCSTR function_name) const;

  
  
  bool GetProcOrdinal(LPCSTR function_name, WORD *ordinal) const;

  
  
  
  bool EnumSections(EnumSectionsFunction callback, PVOID cookie) const;

  
  
  
  bool EnumExports(EnumExportsFunction callback, PVOID cookie) const;

  
  
  
  bool EnumAllImports(EnumImportsFunction callback, PVOID cookie) const;

  
  
  
  bool EnumImportChunks(EnumImportChunksFunction callback, PVOID cookie) const;

  
  
  
  bool EnumOneImportChunk(EnumImportsFunction callback, LPCSTR module_name,
                          PIMAGE_THUNK_DATA name_table, PIMAGE_THUNK_DATA iat,
                          PVOID cookie) const;


  
  
  
  bool EnumAllDelayImports(EnumImportsFunction callback, PVOID cookie) const;

  
  
  
  bool EnumDelayImportChunks(EnumDelayImportChunksFunction callback,
                             PVOID cookie) const;

  
  
  
  bool EnumOneDelayImportChunk(EnumImportsFunction callback,
                               PImgDelayDescr delay_descriptor,
                               LPCSTR module_name,
                               PIMAGE_THUNK_DATA name_table,
                               PIMAGE_THUNK_DATA iat,
                               PIMAGE_THUNK_DATA bound_iat,
                               PIMAGE_THUNK_DATA unload_iat,
                               PVOID cookie) const;

  
  
  
  bool EnumRelocs(EnumRelocsFunction callback, PVOID cookie) const;

  
  
  bool VerifyMagic() const;

  
  virtual PVOID RVAToAddr(DWORD_PTR rva) const;

  
  
  bool ImageRVAToOnDiskOffset(DWORD rva, DWORD *on_disk_offset) const;

  
  
  bool ImageAddrToOnDiskOffset(LPVOID address, DWORD *on_disk_offset) const;

 private:
  HMODULE module_;
};



class PEImageAsData : public PEImage {
 public:
  explicit PEImageAsData(HMODULE hModule) : PEImage(hModule) {}

  virtual PVOID RVAToAddr(DWORD_PTR rva) const;
};

inline bool PEImage::IsOrdinal(LPCSTR name) {
#pragma warning(push)
#pragma warning(disable: 4311)
  
  return reinterpret_cast<DWORD>(name) <= 0xFFFF;
#pragma warning(pop)
}

inline WORD PEImage::ToOrdinal(LPCSTR name) {
  return reinterpret_cast<WORD>(name);
}

inline HMODULE PEImage::module() const {
  return module_;
}

inline PIMAGE_IMPORT_DESCRIPTOR PEImage::GetFirstImportChunk() const {
  return reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
             GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_IMPORT));
}

inline PIMAGE_EXPORT_DIRECTORY PEImage::GetExportDirectory() const {
  return reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
             GetImageDirectoryEntryAddr(IMAGE_DIRECTORY_ENTRY_EXPORT));
}

#endif  
