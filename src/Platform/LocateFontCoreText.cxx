#include "LocateFont.hxx"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#include <optional>

bool LocateFontInit() { return true; }
void LocateFontDeinit() {}

/* TODO: use more than just font family name on macos */
/* TODO: release any allocate data */
std::optional<std::string> LocateFontFile(FontFaceProperties font_face) {
  CFStringRef cf_str = CFStringCreateWithCString(
      nullptr, font_face.family_name.c_str(), kCFStringEncodingUTF8);
  if (cf_str == nullptr)
    return std::nullopt;

  CTFontDescriptorRef font_ref =
      CTFontDescriptorCreateWithNameAndSize(cf_str, font_face.pt_size);
  if (font_ref == nullptr)
    return std::nullopt;

  CFURLRef url =
      (CFURLRef)CTFontDescriptorCopyAttribute(font_ref, kCTFontURLAttribute);
  CFStringRef url_str = CFURLGetString(url);

  return std::string(CFStringGetCStringPtr(url_str, kCFStringEncodingUTF8),
                     CFStringGetLength(url_str));
}
