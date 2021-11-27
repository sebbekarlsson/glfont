#ifndef GLFONT_H
#define GLFONT_H
#include <stdint.h>

#ifndef FREETYPE_SRC
#include <freetype/ftglyph.h>
#include <freetype/ftimage.h>
#include <freetype/ftoutln.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#elif FREETYPE_SRC
include<ft2build.h>
#include <freetype2/freetype/ftglyph.h>
#include <freetype2/freetype/ftimage.h>
#include <freetype2/freetype/ftoutln.h>
#include FT_FREETYPE_H
#endif

#include <cglm/call.h> /* for library call (this also includes cglm.h) */
#include <cglm/cglm.h> /* for inline */

typedef struct GLRAP_VECTOR2 {
  float x;
  float y;
} GLVec2;

typedef struct GLRAP_IVECTOR2 {
  int x;
  int y;
} GLIVec2;

typedef struct GLRAP_UIVECTOR2 {
  int x;
  int y;
} GLUIVec2;

typedef struct GLRAP_VECTOR3 {
  float x;
  float y;
  float z;
} GLVec3;

typedef struct GLFONT_FAMILY_STRUCT {
  char *name;
  FT_Face *face;
  FT_Library ft;
  uint8_t *bytes;
  uint32_t len;
} GLFontFamily;

typedef struct GLFONT_CHARACTER_STRUCT {
  char c;
  float width;
  float height;
  unsigned int advance_x;
  int left;
  int top;
  unsigned int texture;
  float font_size;
  short em_size;
  int descender;
  int ascender;
  float yoffset;
  int zero_height;
  int zero_width;
  float bearing_y;
  GLIVec2 size;
  GLVec2 bearing;
  GLUIVec2 advance;
  float extra;
  FT_GlyphSlot *glyph;
} GLFontCharacter;

typedef struct GL_TEXT_MEASUREMENT_STRUCT {
  float width;
  float height;
  float char_height;
  float char_width;
  int zero_height;
  int zero_width;
} GLTextMeasurement;

typedef struct GLFONT_ATLAS_OPTIONS_STRUCT {
  uint8_t *family_bytes;
  uint32_t len;
  float font_size;
} GLFontAtlasOptions;

typedef struct GLFONT_COLOR_STRUCT {
  float r;
  float g;
  float b;
  float a;
} GLFontColor;

typedef enum {
  GLFONT_GLYPH_MOVE,
  GLFONT_GLYPH_LINE_TO,
  GLFONT_GLYPH_CONIC_TO,
  GLFONT_GLYPH_CONIC_TO_SEQ,
  GLFONT_GLYPH_CUBIC_TO,
  GLFONT_GLYPH_CUBIC_TO_SEQ
} EGlyphPointType;

typedef struct GLFONT_GLYPH_POINT {
  long int x;
  long int y;
  EGlyphPointType type;
  struct GLFONT_GLYPH_POINT *next;
} GLFontGlyphPoint;

void glfont_glyph_point_to_string(char *buff, GLFontGlyphPoint *point);

#define GLFONT_GLYPH_CAP 512

typedef struct GLFONT_GLYPH_STRUCT {
  FT_Matrix matrix;
  FT_Vector delta;
  FT_Glyph glyph;
  GLFontGlyphPoint **points;
  uint32_t points_len;
  uint32_t point_index;
  int pensize;
} GLFontGlyph;

#define GLFONT_COLOR(r, g, b, a) ((GLFontColor){r, g, b, a})

typedef struct GLFONT_TEXT_OPTIONS_STRUCT {
  float font_size;
  GLFontColor color;
  float scale;
  GLFontAtlasOptions atlas_options;
  unsigned int depth_test;
  uint32_t line_width;
  uint32_t letter_spacing;
  unsigned int char_horz_res;
  unsigned int char_vert_res;
  unsigned int pixel_size;
} GLFontTextOptions;

void glfont_load_glyph(GLFontGlyph *glyph, GLFontFamily *family, char c,
                       GLFontTextOptions options);

unsigned int glfont_text_options_is_equal(GLFontTextOptions a,
                                          GLFontTextOptions b);

typedef struct GLFONT_ATLAS_STRUCT {
  unsigned int id;
  uint32_t max_char_height;
  GLFontCharacter **characters;
  uint32_t nr_characters;
  GLFontCharacter *chars[512];
  GLFontFamily *family;

  // cache
  unsigned int VBO;
  unsigned int EBO;
  unsigned int initialized;
  uint32_t nr_rendered_chars;
  GLFontTextOptions options;
  char *text;
  char **text_chunks;
  uint32_t text_chunks_len;
} GLFontAtlas;

void glfont_font_atlas_release_cache(GLFontAtlas *atlas);

void glfont_font_atlas_maybe_release_cache(GLFontAtlas *atlas, const char *text,
                                           GLFontTextOptions options);

GLFontAtlas *glfont_init_atlas(GLFontAtlasOptions options);
GLFontAtlas *glfont_generate_font_atlas_3d(GLFontTextOptions options);

/**
 * Faster when dynamic = 0
 **/
GLFontAtlas *glfont_draw_text_instanced(GLFontAtlas *atlas, const char *text,
                                        float x, float y, float z,
                                        GLFontTextOptions options, mat4 view,
                                        mat4 projection, unsigned int program,
                                        unsigned int dynamic);
GLTextMeasurement *glfont_copy_text_measurement(GLTextMeasurement *measurement);

GLTextMeasurement glfont_get_text_measurement(GLFontCharacter **characters,
                                              uint32_t len);

float glfont_get_text_max_height(GLFontCharacter **characters, uint32_t len);

GLFontFamily *glfont_init_font_family(uint8_t *bytes, uint32_t len);

void glfont_font_family_free(GLFontFamily *family);

void glfont_load_font_character(GLFontCharacter *character,
                                GLFontFamily *family, char c, float font_size,
                                unsigned int horz_res, unsigned int vert_res,
                                unsigned int pixel_size);

void glfont_glyph_free(GLFontGlyph *glyph);
void glfont_glyph_point_free(GLFontGlyphPoint *point);
#endif
