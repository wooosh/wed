#pragma once

#include <optional>
#include <string>

/* TODO: return in memory buffer */
struct FontFaceProperties {
  std::string family_name;
  double pt_size;

  enum Weight {
    WEIGHT_THIN = 100,
    WEIGHT_EXTRA_LIGHT = 200,
    WEIGHT_LIGHT = 300,
    WEIGHT_SEMI_LIGHT = 350,
    WEIGHT_REGULAR = 400,
    WEIGHT_MEDIUM = 500,
    WEIGHT_SEMI_BOLD = 600,
    WEIGHT_BOLD = 700,
    WEIGHT_EXTRA_BOLD = 800,
    WEIGHT_BLACK = 900,
    WEIGHT_EXTRA_BLACK = 950,
  } weight;

  enum Stretch {
    STRETCH_ULTRA_CONDENSED,
    STRETCH_EXTRA_CONDENSED,
    STRETCH_CONDENSED,
    STRETCH_SEMI_CONDENSED,
    STRETCH_MEDIUM,
    STRETCH_SEMI_EXPANDED,
    STRETCH_EXPANDED,
    STRETCH_EXTRA_EXPANDED,
    STRETCH_ULTRA_EXPANDED
  } stretch;

  enum Slant { SLANT_NORMAL, SLANT_ITALIC, SLANT_OBLIQUE } slant;
};

/* Initializes font location service. Returns true on success.
 * THREAD-UNSAFE */
bool LocateFontInit();

/* Deinitializes font location service. Cannot be reinitialized afterwards.
 * THREAD-UNSAFE */
void LocateFontDeinit();

/* Return a path to the best matching font file, if available
 * Requires LocateFontInit to be run once
 * THREAD-UNSAFE */
std::optional<std::string> LocateFontFile(const FontFaceProperties &);