#include "../Util/Assert.hxx"
#include "LocateFont.hxx"
#include <fontconfig/fontconfig.h>
#include <optional>

static int ConvertWeight(enum FontFaceProperties::Weight weight) {
  return FcWeightFromOpenType(weight);
}

static int ConvertStretchType(enum FontFaceProperties::Stretch stretch) {
  using FFP = FontFaceProperties;
  switch (stretch) {
  case FFP::STRETCH_ULTRA_CONDENSED:
    return FC_WIDTH_ULTRACONDENSED;
  case FFP::STRETCH_EXTRA_CONDENSED:
    return FC_WIDTH_EXTRACONDENSED;
  case FFP::STRETCH_CONDENSED:
    return FC_WIDTH_CONDENSED;
  case FFP::STRETCH_SEMI_CONDENSED:
    return FC_WIDTH_SEMICONDENSED;
  case FFP::STRETCH_MEDIUM:;
    return FC_WIDTH_NORMAL;
  case FFP::STRETCH_SEMI_EXPANDED:
    return FC_WIDTH_SEMIEXPANDED;
  case FFP::STRETCH_EXPANDED:
    return FC_WIDTH_EXPANDED;
  case FFP::STRETCH_EXTRA_EXPANDED:
    return FC_WIDTH_EXTRAEXPANDED;
  case FFP::STRETCH_ULTRA_EXPANDED:
    return FC_WIDTH_ULTRAEXPANDED;
  }
  unreachable("unexpected stretch value");
}

static int ConvertSlant(enum FontFaceProperties::Slant slant) {
  using FFP = FontFaceProperties;
  switch (slant) {
  case FFP::SLANT_NORMAL:
    return FC_SLANT_ROMAN;
  case FFP::SLANT_ITALIC:
    return FC_SLANT_ITALIC;
  case FFP::SLANT_OBLIQUE:
    return FC_SLANT_OBLIQUE;
  }
  unreachable("unexpected slant value");
}

static FcConfig *config;

bool LocateFontInit() {
  bool success = FcInit();
  if (!success) {
    return false;
  }

  config = FcInitLoadConfigAndFonts();
  return true;
}

void LocateFontDeinit() {
  FcConfigDestroy(config);
  FcFini();
}

std::optional<std::string> LocateFontFile(const FontFaceProperties &font_face) {
  std::optional<std::string> font_path = std::nullopt;
  FcResult result;
  FcPattern *queried_font;

  /* construct fontconfig pattern*/
  FcPattern *pattern = FcPatternCreate();
  /* Property names:
   * https://www.freedesktop.org/software/fontconfig/fontconfig-devel/x19.html
   */

  bool success;
  success = FcPatternAddString(pattern, FC_FAMILY,
                               (const FcChar8 *)font_face.family_name.c_str());
  if (!success) {
    goto end;
  }

  success = FcPatternAddDouble(pattern, FC_SIZE, font_face.pt_size);
  if (!success) {
    goto end;
  }

  success =
      FcPatternAddInteger(pattern, FC_WEIGHT, ConvertWeight(font_face.weight));
  if (!success) {
    goto end;
  }

  success = FcPatternAddInteger(pattern, FC_WIDTH,
                                ConvertStretchType(font_face.stretch));
  if (!success) {
    goto end;
  }

  success =
      FcPatternAddInteger(pattern, FC_SLANT, ConvertSlant(font_face.slant));
  if (!success) {
    goto end;
  }

  /* apply their config settings to the pattern */
  success = FcConfigSubstitute(config, pattern, FcMatchPattern);
  if (!success) {
    goto end;
  }

  /* fill in default pattern values if neccesary */
  FcDefaultSubstitute(pattern);

  /* query for font */

  queried_font = FcFontMatch(config, pattern, &result);
  if (result == FcResultOutOfMemory || result == FcResultNoMatch ||
      queried_font == nullptr) {
    goto end_query;
  }

  FcChar8 *cstr_path;
  if (FcPatternGetString(queried_font, FC_FILE, 0, &cstr_path) !=
      FcResultMatch) {
    goto end_query;
  }

  font_path = std::string((char *)cstr_path);

end_query:
  FcPatternDestroy(queried_font);

end:
  FcPatternDestroy(pattern);
  return font_path;
}