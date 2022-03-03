#include "LocateFont.hxx"
#include "SDL_video.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#include <optional>

bool LocateFontInit() { return true; }
void LocateFontDeinit() {}

/* TODO: use more than just font family name on macos */
/* TODO: release any allocate data */
std::optional<std::string>
LocateFontFile(const FontFaceProperties &font_face) {
  CFStringRef cf_str = CFStringCreateWithCString(
      nullptr, font_face.family_name.c_str(), kCFStringEncodingUTF8);
  if (cf_str == nullptr)
    return std::nullopt;

  CTFontDescriptorRef font_ref =
      CTFontDescriptorCreateWithNameAndSize(cf_str, font_face.pt_size);
  if (font_ref == nullptr) {
    CFRelease(cf_str);
    return std::nullopt;
  }

  CFURLRef url =
      (CFURLRef)CTFontDescriptorCopyAttribute(font_ref, kCTFontURLAttribute);
  if (url == nullptr) {
    CFRelease(cf_str);
    CFRelease(font_ref);
    return std::nullopt;
  }

  CFStringRef url_str = CFURLGetString(url);
  url_str = (CFStringRef)CFRetain(url_str);

  std::string path(CFStringGetCStringPtr(url_str, kCFStringEncodingUTF8),
                   CFStringGetLength(url_str));

  CFRelease(url_str);
  CFRelease(font_ref);
  CFRelease(cf_str);
  return path;
}
