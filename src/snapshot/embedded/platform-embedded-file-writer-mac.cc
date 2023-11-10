// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/snapshot/embedded/platform-embedded-file-writer-mac.h"

namespace v8 {
namespace internal {

namespace {

const char* DirectiveAsString(DataDirective directive) {
  switch (directive) {
    case kByte:
      return ".byte";
    case kLong:
      return ".long";
    case kQuad:
      return ".quad";
#ifndef __POWERPC__
    case kOcta:
      return ".octa";
#endif
  }
  UNREACHABLE();
}

}  // namespace

void PlatformEmbeddedFileWriterMac::SectionText() { fprintf(fp_, ".text\n"); }

void PlatformEmbeddedFileWriterMac::SectionData() { fprintf(fp_, ".data\n"); }

void PlatformEmbeddedFileWriterMac::SectionRoData() {
  fprintf(fp_, ".const_data\n");
}

void PlatformEmbeddedFileWriterMac::DeclareUint32(const char* name,
                                                  uint32_t value) {
  DeclareSymbolGlobal(name);
  DeclareLabel(name);
  IndentedDataDirective(kLong);
  fprintf(fp_, "%d", value);
  Newline();
}

void PlatformEmbeddedFileWriterMac::DeclarePointerToSymbol(const char* name,
                                                           const char* target) {
  DeclareSymbolGlobal(name);
  DeclareLabel(name);
  fprintf(fp_, "  %s _%s\n", DirectiveAsString(PointerSizeDirective()), target);
}

void PlatformEmbeddedFileWriterMac::DeclareSymbolGlobal(const char* name) {
#ifdef __POWERPC__
  // Follow AIX here.
  fprintf(fp_, ".globl _%s\n", name);
#else
  // TODO(jgruber): Investigate switching to .globl. Using .private_extern
  // prevents something along the compilation chain from messing with the
  // embedded blob. Using .global here causes embedded blob hash verification
  // failures at runtime.
  fprintf(fp_, ".private_extern _%s\n", name);
#endif
}

void PlatformEmbeddedFileWriterMac::AlignToCodeAlignment() {
#ifdef __POWERPC__
  fprintf(fp_, ".align 5\n");
#else
  fprintf(fp_, ".balign 32\n");
#endif
}

void PlatformEmbeddedFileWriterMac::AlignToDataAlignment() {
#ifdef __POWERPC__
  fprintf(fp_, ".align 3\n");
#else
  fprintf(fp_, ".balign 8\n");
#endif
}

void PlatformEmbeddedFileWriterMac::Comment(const char* string) {
#ifdef __POWERPC__
  fprintf(fp_, "; %s\n", string);
#else
  fprintf(fp_, "// %s\n", string);
#endif
}

void PlatformEmbeddedFileWriterMac::DeclareLabel(const char* name) {
  fprintf(fp_, "_%s:\n", name);
}

void PlatformEmbeddedFileWriterMac::SourceInfo(int fileid, const char* filename,
                                               int line) {
#ifdef __POWERPC__
  fprintf(fp_, ".line %d, \"%s\"\n", line, filename);
#else
  fprintf(fp_, ".loc %d %d\n", fileid, line);
#endif
}

// TODO(mmarchini): investigate emitting size annotations for OS X
void PlatformEmbeddedFileWriterMac::DeclareFunctionBegin(const char* name,
                                                         uint32_t size) {
  DeclareLabel(name);
  // TODO(mvstanton): Investigate the proper incantations to mark the label as
  // a function on OSX.
}

void PlatformEmbeddedFileWriterMac::DeclareFunctionEnd(const char* name) {}

int PlatformEmbeddedFileWriterMac::HexLiteral(uint64_t value) {
  return fprintf(fp_, "0x%" PRIx64, value);
}

void PlatformEmbeddedFileWriterMac::FilePrologue() {}

void PlatformEmbeddedFileWriterMac::DeclareExternalFilename(
    int fileid, const char* filename) {
  fprintf(fp_, ".file %d \"%s\"\n", fileid, filename);
}

void PlatformEmbeddedFileWriterMac::FileEpilogue() {}

int PlatformEmbeddedFileWriterMac::IndentedDataDirective(
    DataDirective directive) {
  return fprintf(fp_, "  %s ", DirectiveAsString(directive));
}

#ifdef __POWERPC__

DataDirective PlatformEmbeddedFileWriterMac::ByteChunkDataDirective() const {
  // PPC uses a fixed 4 byte instruction set, using .long
  // to prevent any unnecessary padding.
  return kLong;
}

int PlatformEmbeddedFileWriterMac::WriteByteChunk(const uint8_t* data) {
  DCHECK_EQ(ByteChunkDataDirective(), kLong);
  const uint32_t* long_ptr = reinterpret_cast<const uint32_t*>(data);
  return HexLiteral(*long_ptr);
}

#endif // __POWERPC__

}  // namespace internal
}  // namespace v8
