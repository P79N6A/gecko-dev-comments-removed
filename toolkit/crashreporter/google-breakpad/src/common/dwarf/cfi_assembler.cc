

































#include "common/dwarf/cfi_assembler.h"

#include <assert.h>
#include <stdlib.h>

namespace google_breakpad {

using dwarf2reader::DwarfPointerEncoding;
  
CFISection &CFISection::CIEHeader(uint64_t code_alignment_factor,
                                  int data_alignment_factor,
                                  unsigned return_address_register,
                                  uint8_t version,
                                  const string &augmentation,
                                  bool dwarf64) {
  assert(!entry_length_);
  entry_length_ = new PendingLength();
  in_fde_ = false;

  if (dwarf64) {
    D32(kDwarf64InitialLengthMarker);
    D64(entry_length_->length);
    entry_length_->start = Here();
    D64(eh_frame_ ? kEHFrame64CIEIdentifier : kDwarf64CIEIdentifier);
  } else {
    D32(entry_length_->length);
    entry_length_->start = Here();
    D32(eh_frame_ ? kEHFrame32CIEIdentifier : kDwarf32CIEIdentifier);
  }
  D8(version);
  AppendCString(augmentation);
  ULEB128(code_alignment_factor);
  LEB128(data_alignment_factor);
  if (version == 1)
    D8(return_address_register);
  else
    ULEB128(return_address_register);
  return *this;
}

CFISection &CFISection::FDEHeader(Label cie_pointer,
                                  uint64_t initial_location,
                                  uint64_t address_range,
                                  bool dwarf64) {
  assert(!entry_length_);
  entry_length_ = new PendingLength();
  in_fde_ = true;
  fde_start_address_ = initial_location;

  if (dwarf64) {
    D32(0xffffffff);
    D64(entry_length_->length);
    entry_length_->start = Here();
    if (eh_frame_)
      D64(Here() - cie_pointer);
    else
      D64(cie_pointer);
  } else {
    D32(entry_length_->length);
    entry_length_->start = Here();
    if (eh_frame_)
      D32(Here() - cie_pointer);
    else
      D32(cie_pointer);
  }
  EncodedPointer(initial_location);
  
  
  
  
  EncodedPointer(address_range,
                 DwarfPointerEncoding(pointer_encoding_ & 0x0f));
  return *this;
}

CFISection &CFISection::FinishEntry() {
  assert(entry_length_);
  Align(address_size_, dwarf2reader::DW_CFA_nop);
  entry_length_->length = Here() - entry_length_->start;
  delete entry_length_;
  entry_length_ = NULL;
  in_fde_ = false;
  return *this;
}

CFISection &CFISection::EncodedPointer(uint64_t address,
                                       DwarfPointerEncoding encoding,
                                       const EncodedPointerBases &bases) {
  
  if (encoding == dwarf2reader::DW_EH_PE_omit)
    return *this;

  
  
  
  encoding = DwarfPointerEncoding(encoding & ~dwarf2reader::DW_EH_PE_indirect);

  
  
  uint64_t base;
  switch (encoding & 0xf0) {
    case dwarf2reader::DW_EH_PE_absptr:  base = 0;                  break;
    case dwarf2reader::DW_EH_PE_pcrel:   base = bases.cfi + Size(); break;
    case dwarf2reader::DW_EH_PE_textrel: base = bases.text;         break;
    case dwarf2reader::DW_EH_PE_datarel: base = bases.data;         break;
    case dwarf2reader::DW_EH_PE_funcrel: base = fde_start_address_; break;
    case dwarf2reader::DW_EH_PE_aligned: base = 0;                  break;
    default: abort();
  };

  
  
  address -= base;

  
  if ((encoding & 0xf0) == dwarf2reader::DW_EH_PE_aligned)
    Align(AddressSize());

  
  
  
  
  switch (encoding & 0x0f) {
    case dwarf2reader::DW_EH_PE_absptr:
      Address(address);
      break;

    case dwarf2reader::DW_EH_PE_uleb128:
      ULEB128(address);
      break;

    case dwarf2reader::DW_EH_PE_sleb128:
      LEB128(address);
      break;

    case dwarf2reader::DW_EH_PE_udata2:
    case dwarf2reader::DW_EH_PE_sdata2:
      D16(address);
      break;

    case dwarf2reader::DW_EH_PE_udata4:
    case dwarf2reader::DW_EH_PE_sdata4:
      D32(address);
      break;

    case dwarf2reader::DW_EH_PE_udata8:
    case dwarf2reader::DW_EH_PE_sdata8:
      D64(address);
      break;

    default:
      abort();
  }

  return *this;
};

const uint32_t CFISection::kDwarf64InitialLengthMarker;
const uint32_t CFISection::kDwarf32CIEIdentifier;
const uint64_t CFISection::kDwarf64CIEIdentifier;
const uint32_t CFISection::kEHFrame32CIEIdentifier;
const uint64_t CFISection::kEHFrame64CIEIdentifier;

} 
