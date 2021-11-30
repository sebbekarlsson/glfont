#include <GL/glew.h>
#include <glfont/glfont.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <GL/glew.h>
#include <glfont/glfont.h>
#include <glfont/macros.h>
#include <glfont/string_utils.h>
#include <math.h>

#define WRAPS GL_CLAMP_TO_BORDER
#define FT_FLAGS FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL
#define DPI_H 96
#define DPI_V 96
#define PEN_SIZE 64

#define FIRST_CHAR 32  // start of the range of possible characters we support
#define NUM_GLYPHS 128 // how many characters we support

// how many characters we render per call (chunk length)
#define STRING_BUFFER_CAP 256

static const char *getErrorMessage(FT_Error err) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)                                                   \
  case e:                                                                      \
    return s;
#define FT_ERROR_START_LIST switch (err) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
  return "(Unknown error)";
}

GLFontFamily *glfont_init_font_family(uint8_t *bytes, uint32_t len) {
  GLFontFamily *family = NEW(GLFontFamily);

  family->face = NEW(FT_Face);
  FT_Error err;
  if ((err = FT_Init_FreeType(&family->ft))) {
    printf("Could not load freetype library.\n");
    printf("%s\n", getErrorMessage(err));
    return 0;
  }

  if ((err = FT_New_Memory_Face(family->ft, bytes, len, 0, family->face))) {
    printf("Could not load family.\n");
    printf("%s\n", getErrorMessage(err));
    return 0;
  }

  family->bytes = bytes;
  family->len = len;

  return family;
}
void glfont_font_family_free(GLFontFamily *family) {
  if (family->name != 0)
    free(family->name);
  FT_Done_Face(*family->face);
  FT_Done_FreeType(family->ft);
  free(family);
}

void glfont_load_font_character(GLFontCharacter *character,
                                GLFontFamily *family, char c, float font_size,
                                unsigned int horz_res, unsigned int vert_res,
                                unsigned int pixel_size) {
  int f = (int)ceil(font_size);
  FT_Set_Char_Size(*family->face, 0, ceil(f * OR(pixel_size, PEN_SIZE)),
                   horz_res, vert_res);

  FT_Face face = *family->face;
  int errcode = 0;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  if ((errcode = FT_Load_Char(face, c, FT_FLAGS))) {
    printf("Could not load character %c (error %d).\n", c, errcode);
    exit(1);
    return;
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  const short em_size = face->ascender - face->descender;
  const float descender = ceil((float)face->descender / (float)em_size);
  const float ascender = ceil((float)1.0f - descender);

  FT_Glyph glyph;
  FT_Get_Glyph(face->glyph, &glyph);
  FT_BBox bbox;
  FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_UNSCALED, &bbox);
  const float yoffset = (float)((float)bbox.yMin);

  glGenTextures(1, &character->texture);
  glBindTexture(GL_TEXTURE_2D, character->texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WRAPS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, WRAPS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, face->glyph->bitmap.width,
               face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
               face->glyph->bitmap.buffer);

  character->glyph = &face->glyph;
  character->width = face->glyph->bitmap.width;
  character->height = face->glyph->bitmap.rows;
  character->left = (int)ceil(face->glyph->bitmap_left);
  character->top = (int)ceil(face->glyph->bitmap_top);
  character->advance_x = (unsigned int)face->glyph->advance.x;
  character->advance =
      (GLUIVec2){face->glyph->advance.x, face->glyph->advance.y};
  character->c = c;
  character->em_size = em_size;
  character->descender = (int)ceil(descender);
  character->yoffset = yoffset;
  character->ascender = (int)ceil(ascender);
  character->bearing_y = face->glyph->metrics.horiBearingY;
  character->size =
      (GLIVec2){face->glyph->bitmap.width, face->glyph->bitmap.rows};
  character->bearing =
      (GLVec2){face->glyph->bitmap_left, face->glyph->bitmap_top};

  {
    int errcode = 0;
    if ((errcode = FT_Load_Char(face, '0', FT_FLAGS))) {
      printf("Could not load character 0 (error %d).\n", errcode);
      exit(1);
      return;
    }
    character->width = fmax(character->width, face->glyph->bitmap.width);
    character->zero_height = face->glyph->bitmap.rows;
    character->zero_width = face->glyph->bitmap.width;
  }
}

GLFontAtlas *glfont_generate_font_atlas_3d(GLFontTextOptions options) {

  GLFontFamily *family = glfont_init_font_family(
      options.atlas_options.family_bytes, options.atlas_options.len);
  FT_Face face = *family->face;
  FT_Library ft = family->ft;

  GLFontAtlas *atlas = NEW(GLFontAtlas);
  atlas->family = family;

  GLuint texid = 0;
  glGenTextures(1, &texid);
  atlas->id = texid;

  glBindTexture(GL_TEXTURE_3D, atlas->id);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindImageTexture(0, atlas->id, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32I);
  glGenerateMipmap(GL_TEXTURE_3D);

  unsigned int nr_chars = 128;

  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, WRAPS);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, WRAPS);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int f = (int)ceil(options.font_size);

  unsigned int px = OR(options.pixel_size, PEN_SIZE);

  FT_Set_Char_Size(face, 0, f * px, OR(options.char_horz_res, DPI_H),
                   OR(options.char_vert_res, DPI_V));

  int imageWidth = 0;
  int imageHeight = 0;
  int maxAscent = 0;
  for (unsigned int c = FIRST_CHAR; c < 128; c++) {
    int errcode = 0;
    if ((errcode = FT_Load_Char(face, (char)c, FT_FLAGS)))
      continue;

    FT_GlyphSlot glyph = face->glyph;
    if (glyph->bitmap_top > (int)maxAscent) {
      maxAscent = glyph->bitmap_top;
    }
    FT_BBox bbox;
    FT_Glyph glyph2;
    FT_Get_Glyph(face->glyph, &glyph2);
    FT_Glyph_Get_CBox(glyph2, FT_GLYPH_BBOX_UNSCALED, &bbox);

    int gw = ceil(glyph->metrics.width / px) + glyph->bitmap_left;

    int hh = ceil(glyph->metrics.height / px) + ceil(glyph->bitmap_top / 4);

    if (hh > imageHeight) {
      imageHeight = hh;
    }

    if (gw > imageWidth)
      imageWidth = gw;
  }

  glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGB8, imageWidth, imageHeight, nr_chars);

  for (GLsizei i = FIRST_CHAR; i < nr_chars; i++) {
    int errcode = 0;
    if ((errcode = FT_Load_Char(face, (char)i, FT_FLAGS))) {
      printf("Could not load character %c (error %d).\n", (char)i, errcode);
      exit(1);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, WRAPS);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, WRAPS);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    FT_Glyph glyph;
    FT_Get_Glyph(face->glyph, &glyph);
    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_UNSCALED, &bbox);

    int w = face->glyph->bitmap.width;
    int h = face->glyph->bitmap.rows;
    float y = ceil(maxAscent - face->glyph->bitmap_top);
    float xx = ceil(face->glyph->bitmap_left);

    float bearingX = face->glyph->metrics.horiBearingX / px; // position
    float scalar_x = 1.0f / (float)f;
    glGenerateMipmap(GL_TEXTURE_3D);
    glTexSubImage3D(GL_TEXTURE_3D, 0, -xx / (px / 2), y, i - FIRST_CHAR, w, h,
                    1.0f, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
    glGenerateMipmap(GL_TEXTURE_3D);

    glBindTexture(GL_TEXTURE_3D, 0);
    GLFontCharacter *character = NEW(GLFontCharacter);
    glfont_load_font_character(
        character, family, (char)i, options.font_size,
        OR(options.char_horz_res, DPI_H), // horizontal dpi
        OR(options.char_vert_res, DPI_V), // vertical dpi
        OR(options.pixel_size, PEN_SIZE));

    character->extra = imageWidth - (w);
    atlas->chars[(int)i] = character;

    glBindTexture(GL_TEXTURE_3D, atlas->id);
    glBindImageTexture(0, atlas->id, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32I);
  }

  glBindTexture(GL_TEXTURE_3D, 0);
  return atlas;
}

void glw_font_atlas_release_cache(GLFontAtlas *atlas) {
  if (!atlas->initialized)
    return;
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &atlas->VBO);
  glDeleteBuffers(1, &atlas->EBO);
  atlas->VBO = 0;
  atlas->EBO = 0;

  if (atlas->text != 0) {
    free(atlas->text);
    atlas->text = 0;
  }

  atlas->initialized = 0;
}

unsigned int glfont_text_options_is_equal(GLFontTextOptions a,
                                          GLFontTextOptions b) {
  unsigned int color_eq = a.color.r == b.color.r && a.color.g == b.color.g &&
                          a.color.b == b.color.b && a.color.a == b.color.a;
  unsigned int font_size_eq = a.font_size == b.font_size;
  unsigned int scale_eq = a.scale == b.scale;
  return color_eq && font_size_eq && scale_eq;
}

void glfont_atlas_maybe_release_cache(GLFontAtlas *atlas, const char *text,
                                      GLFontTextOptions options) {
  if ((text && atlas->text && strcmp(text, atlas->text) != 0) ||
      !glfont_text_options_is_equal(atlas->options, options)) {
    return glw_font_atlas_release_cache(atlas);
  }
}

static void glfont_buffer_quad(GLFontAtlas *atlas, unsigned int program,
                               float width, float height, GLFontColor color,
                               mat4 view, mat4 projection,
                               uint32_t array_offset, uint32_t element_offset) {

  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, *view);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE,
                     *projection);

  float w = width;
  float h = height;
  float r = color.r;
  float g = color.g;
  float b = color.b;
  float a = color.a;
  int K = 0;

  float vertices[] = {0 + 0.0f, 0 + h, 0.0f, r, g, b, a, 0.0f, 0.0f,
                      0 + w,    0 + h, 0.0f, r, g, b, a, 1.0f, 0.0f,
                      0 + w,    0 + 0,    0.0f, r, g, b, a, 1.0f, 1.0f,
                      0 + 0.0f, 0 + 0,    0.0f, r, g, b, a, 0.0f, 1.0f};

  unsigned int indices[] = {0 + K, 1 + K, 3 + K, 1 + K, 2 + K, 3 + K};

  glUniform4f(glGetUniformLocation(program, "color"), color.r, color.g, color.b,
              color.a);

  glBindBuffer(GL_ARRAY_BUFFER, atlas->VBO);
  glBufferSubData(GL_ARRAY_BUFFER, array_offset, sizeof(vertices), vertices);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, atlas->EBO);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, element_offset, sizeof(indices),
                  indices);

  // x, y, z
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        (void *)0 + array_offset);
  glEnableVertexAttribArray(0);

  // r, g, b, a
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        (void *)(array_offset + (3 * sizeof(float))));
  glEnableVertexAttribArray(1);

  // texcoords
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                        (void *)(array_offset + (7 * sizeof(float))));
  glEnableVertexAttribArray(2);
}

GLFontAtlas *glfont_draw_text_instanced(GLFontAtlas *atlas, const char *text,
                                        float x, float y, float z,
                                        GLFontTextOptions options, mat4 view,
                                        mat4 projection, unsigned int program,
                                        unsigned int dynamic) {

  if (!atlas) {
    atlas = glfont_generate_font_atlas_3d(options);
  } else if (dynamic) {
    glfont_atlas_maybe_release_cache(atlas, text, options);
  }

  int scale = 1;

  float rx = 0;
  float extra_y = 0;

  if (!atlas->initialized) {
    atlas->nr_rendered_chars = strlen(text);

    atlas->text_chunks =
        glfont_str_chunk(text, STRING_BUFFER_CAP, &atlas->text_chunks_len);

    glGenBuffers(1, &atlas->VBO);
    glGenBuffers(1, &atlas->EBO);
    atlas->initialized = 1;
  }

  uint32_t nr_chunks = atlas->text_chunks_len;

  uint32_t current_line_width = 0;

  // TODO: make this work to avoid chunks.
  //
  // glBindBuffer(GL_ARRAY_BUFFER, atlas->VBO);
  // kGLuint ssbo;
  // glGenBuffers(1, &ssbo);
  // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
  // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
  // glBufferData(GL_SHADER_STORAGE_BUFFER, strlen(text) * (sizeof(float) * 3),
  // 0, GL_STATIC_DRAW); //sizeof(data) only works for statically sized C/C++
  // arrays.

  uint32_t buffered_chunks = 0;

  for (uint32_t ci = 0; ci < nr_chunks; ci++) {

    char *chunk = atlas->text_chunks[ci];
    uint32_t textlen = strlen(chunk); // atlas->nr_rendered_chars;

    /** Allocate buffer memory */

    glBindBuffer(GL_ARRAY_BUFFER, atlas->VBO);
    glBufferData(GL_ARRAY_BUFFER, 9 * 4 * textlen * sizeof(float), 0,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, atlas->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * textlen * sizeof(float), 0,
                 GL_STATIC_DRAW);

    uint32_t array_offset = 0;
    uint32_t element_offset = 0;
    glBindTexture(GL_TEXTURE_3D, atlas->id);

    int imageWidth = 0;
    int maxAscent = 0;
    for (unsigned int c = FIRST_CHAR; c < 128; c++) {
      GLFontCharacter *ch = (GLFontCharacter *)atlas->chars[(int)c];

      FT_GlyphSlot glyph = *ch->glyph;
      if (glyph->bitmap_top > (int)maxAscent) {
        maxAscent = glyph->bitmap_top;
      }
    }

    int i = 0;
    char c = 0;
    float xpos = 0;
    float ypos = 0;
    uint32_t buffered_characters = 0;
    for (i = 0; i < textlen; i++) {
      char c = chunk[i];

      if (c == '\n') {
        extra_y += options.font_size;
        rx = 0;
      }

      GLFontCharacter *ch = (GLFontCharacter *)atlas->chars[(int)c];
      if (ch == 0 && i < textlen) {
        continue;
      };

      FT_GlyphSlot glyph = *ch->glyph;

      FT_Glyph glyphK;
      FT_Get_Glyph(glyph, &glyphK);
      FT_BBox bbox;
      FT_Glyph_Get_CBox(glyphK, FT_GLYPH_BBOX_GRIDFIT, &bbox);

      float width = glyph->metrics.width / OR(options.pixel_size, PEN_SIZE);
      float height = glyph->metrics.height / OR(options.pixel_size, PEN_SIZE);

      int w = ch->size.x * scale;
      int h = ch->size.y * scale;

      ypos = extra_y;
      xpos = (rx + ch->bearing.x);

      if (ch->c != '\r' && ch->c != '\n') {

        int f = (int)ceil(options.font_size);

        float scalar_z = 1.0f / (float)128 - FIRST_CHAR;
        float stack_index = (float)((double)c - FIRST_CHAR) * scalar_z;
        float sx = (1.0f / w);

        GLFontColor color = options.color;

        /**  Buffer quad */
        {
          if (width <= 0 || height <= 0 || program == 0)
            continue;

          glfont_buffer_quad(atlas, program, width, height, color, view,
                             projection, array_offset, element_offset);

          char buff[256];
          sprintf(buff, "offsets[%d]", (int)i);
          glUniform3f(glGetUniformLocation(program, buff), ceil(xpos),
                      ceil(ypos), 0);

          char buff2[256];
          sprintf(buff2, "toff[%d]", (int)i);
          glUniform3f(glGetUniformLocation(program, buff2), 0, 0, stack_index);

          // TODO: make this work to avoid chunks
          //
          // glUniform1i(glGetUniformLocation(program, "myId"), (int)i);
          // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
          // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
          // float off[] = { (float)ceil(xpos), (float)ceil(ypos), 0.0f };
          // glBufferSubData(GL_SHADER_STORAGE_BUFFER, vec3_offset_bytes,
          // sizeof(float)*3, &off);

          array_offset += (9 * 4 * sizeof(float));
          element_offset += (6 * sizeof(float));
          current_line_width += ch->size.x;

          buffered_characters += 1;
        }
      }

      // is the text overflowing?
      unsigned int overflow =
          options.line_width
              ? (xpos + w + options.letter_spacing) > options.line_width
              : 0;

      if (ch->c == '\n') {
        rx = x;
      } else {
        (rx += (ch->advance_x) >>
               6); // the amount the character wants to move horizontally
        rx -= ch->size.x /
              2; // remove half of the size from the x position, because of how
                 // the atlas works (haven't figured out exactly why yet)
        rx += options.letter_spacing; // add user definied letter spacing
      }

      if (overflow && ch->c != '\r' && ch->c != '\n' && ch->c != 13 &&
          ch->c != 10 && ch->c != ' ' && ch->size.x > 0 && ch->size.y > 0) {
        current_line_width = 0;
        rx = 0;
        extra_y += options.font_size; // text is overflowing, so let's move the
                                      // cursor down
      }
    }

    if (options.depth_test) {
      glEnable(GL_DEPTH_TEST);
    } else {
      glDisable(GL_DEPTH_TEST);
    }
    // TODO: make this work to avoid chunks
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    glBindTexture(GL_TEXTURE_3D, atlas->id);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(0, atlas->id, 0, /*layered=*/GL_TRUE, 0, GL_READ_WRITE,
                       GL_R32I);
    glDrawElementsInstanced(GL_TRIANGLES, textlen, GL_UNSIGNED_INT, 0, textlen);
    // TODO: make this work to avoid chunks
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
    buffered_chunks += 1;
  }

  glBindVertexArray(0);

  return atlas;
}

float glfont_get_text_max_height(GLFontCharacter **characters, uint32_t len) {

  float h = 0;
  for (uint32_t i = 0; i < len; i++) {
    void *ptr = characters[i];
    GLFontCharacter *character = (GLFontCharacter *)ptr;

    if (character->height > h) {
      h = character->height;
    }
  }

  return h;
}

GLTextMeasurement glfont_get_text_measurement(GLFontCharacter **characters,
                                              uint32_t len) {
  float max_w = 0;
  float max_h = glfont_get_text_max_height(characters, len);
  float h = max_h;
  float line_w = 0;
  int zero_height = 0;
  int zero_width = 0;

  for (uint32_t i = 0; i < len; i++) {
    void *ptr = characters[i];
    GLFontCharacter *character = (GLFontCharacter *)ptr;

    if (character->zero_height > zero_height) {
      zero_height = character->zero_height;
    }

    if (character->zero_width > zero_width) {
      zero_width = character->zero_width;
    }

    if (character->c == '\r' || character->c == '\n') {
      h += max_h;

      if (line_w > max_w) {
        max_w = line_w;
      }

      line_w = 0;
    } else {
      line_w += character->width;
    }
  }

  return (GLTextMeasurement){
      max_w == 0 ? (line_w) : max_w, h, max_h, max_w, zero_height, zero_width};
}

GLTextMeasurement *
glfont_copy_text_measurement(GLTextMeasurement *measurement) {
  GLTextMeasurement *m = NEW(GLTextMeasurement);
  m->width = measurement->width;
  m->height = measurement->height;
  m->char_height = measurement->char_height;
  m->zero_height = measurement->zero_height;
  m->zero_width = measurement->zero_width;
  return m;
}

static int moveTo(FT_Vector *to, void *ptr) {
  GLFontGlyph *glyph = (GLFontGlyph *)ptr;
  GLFontGlyphPoint *point = NEW(GLFontGlyphPoint);
  point->type = GLFONT_GLYPH_MOVE;
  point->x = to->x;
  point->y = to->y;
  glyph->point_index += 1;
  glyph->points = (GLFontGlyphPoint **)realloc(
      glyph->points, glyph->point_index * sizeof(GLFontGlyphPoint *));
  glyph->points[glyph->point_index - 1] = point;
  return 0;
}

static int lineTo(FT_Vector *to, void *ptr) {
  GLFontGlyph *glyph = (GLFontGlyph *)ptr;
  GLFontGlyphPoint *point = NEW(GLFontGlyphPoint);
  point->type = GLFONT_GLYPH_LINE_TO;
  point->x = to->x;
  point->y = to->y;

  glyph->point_index += 1;
  glyph->points = (GLFontGlyphPoint **)realloc(
      glyph->points, glyph->point_index * sizeof(GLFontGlyphPoint *));
  glyph->points[glyph->point_index - 1] = point;
  return 0;
}

static int conicTo(FT_Vector *control, FT_Vector *to, void *ptr) {
  GLFontGlyph *glyph = (GLFontGlyph *)ptr;

  GLFontGlyphPoint *point = NEW(GLFontGlyphPoint);
  point->type = GLFONT_GLYPH_CONIC_TO;
  point->x = control->x / glyph->pensize;
  point->y = -control->y / glyph->pensize;

  GLFontGlyphPoint *point2 = NEW(GLFontGlyphPoint);
  point2->type = GLFONT_GLYPH_CONIC_TO_SEQ;
  point2->x = to->x / glyph->pensize;
  point2->y = -to->y / glyph->pensize;
  point->next = point2;

  glyph->point_index += 1;
  glyph->points = (GLFontGlyphPoint **)realloc(
      glyph->points, glyph->point_index * sizeof(GLFontGlyphPoint *));
  glyph->points[glyph->point_index - 1] = point;

  return 0;
}

static int cubicTo(FT_Vector *control1, FT_Vector *control2, FT_Vector *to,
                   void *ptr) {
  GLFontGlyph *glyph = (GLFontGlyph *)ptr;

  GLFontGlyphPoint *point = NEW(GLFontGlyphPoint);
  point->type = GLFONT_GLYPH_CUBIC_TO;
  point->x = control1->x / glyph->pensize;
  point->y = -control1->y / glyph->pensize;

  GLFontGlyphPoint *point2 = NEW(GLFontGlyphPoint);
  point2->type = GLFONT_GLYPH_CUBIC_TO_SEQ;
  point2->x = control2->x / glyph->pensize;
  point2->y = -control2->y / glyph->pensize;
  point->next = point2;

  GLFontGlyphPoint *point3 = NEW(GLFontGlyphPoint);
  point3->type = GLFONT_GLYPH_CUBIC_TO_SEQ;
  point3->x = to->x / glyph->pensize;
  point3->y = -to->y / glyph->pensize;
  point2->next = point3;

  glyph->point_index += 1;
  glyph->points = (GLFontGlyphPoint **)realloc(
      glyph->points, glyph->point_index * sizeof(GLFontGlyphPoint *));

  return 0;
}

void glfont_load_glyph(GLFontGlyph *glyph, GLFontFamily *family, char c,
                       GLFontTextOptions options) {
  GLFontCharacter character = {};
  int pensize = OR(options.pixel_size, PEN_SIZE);
  glyph->pensize = pensize;
  glfont_load_font_character(&character, family, c, options.font_size,
                             OR(options.char_horz_res, DPI_H),
                             OR(options.char_vert_res, DPI_V), pensize);

  FT_Glyph glyph1;
  FT_Get_Glyph(*character.glyph, &glyph1);
  FT_Matrix matrix;

  /* copy glyph to glyph2 */
  int error = FT_Glyph_Copy(glyph1, &glyph->glyph);
  if (error) {
    printf("Could not copy\n");
  }

  /* translate "glyph" */
  glyph->delta.x = -100 * pensize; /* coordinates are in 26.6 pixels */
  glyph->delta.y = 50 * pensize;

  FT_Glyph_Transform(glyph->glyph, 0, &glyph->delta);

  /* transform glyph2 (horizontal shear) */
  matrix.xx = 0x10000L;
  matrix.xy = 0;
  matrix.yx = 0.12 * 0x10000L;
  matrix.yy = 0x10000L;

  FT_Outline_Funcs funcs;
  funcs.move_to = (FT_Outline_MoveTo_Func)&moveTo;
  funcs.line_to = (FT_Outline_LineTo_Func)&lineTo;
  funcs.conic_to = (FT_Outline_ConicTo_Func)&conicTo;
  funcs.cubic_to = (FT_Outline_CubicTo_Func)&cubicTo;
  funcs.shift = 0;
  funcs.delta = 0;

  FT_Outline outline = (*character.glyph)->outline;
  FT_Outline_Decompose(&outline, &funcs, glyph);

  FT_Glyph_Transform(glyph->glyph, &matrix, 0);

  glyph->points_len = glyph->point_index - 1;
}

void glfont_glyph_free(GLFontGlyph *glyph) {
  if (glyph->points) {
    for (uint32_t i = 0; i < glyph->points_len; i++) {
      glfont_glyph_point_free(glyph->points[i]);
    }
  }

  free(glyph);
}

void glfont_glyph_point_free(GLFontGlyphPoint *point) {
  if (point->next != 0)
    glfont_glyph_point_free(point->next);
  free(point);
}

void glfont_glyph_point_to_string(char *buff, GLFontGlyphPoint *point) {
  const char *typename = 0;
  switch (point->type) {
  case GLFONT_GLYPH_MOVE:
    typename = "GLFONT_GLYPH_MOVE";
    break;
  case GLFONT_GLYPH_LINE_TO:
    typename = "GLFONT_GLYPH_LINE_TO";
    break;
  case GLFONT_GLYPH_CUBIC_TO:
    typename = "GLFONT_GLYPH_CUBIC_TO";
    break;
  case GLFONT_GLYPH_CONIC_TO:
    typename = "GLFONT_GLYPH_CONIC_TO";
    break;
  case GLFONT_GLYPH_CUBIC_TO_SEQ:
    typename = "GLFONT_GLYPH_CUBIC_TO_SEQ";
    break;
  case GLFONT_GLYPH_CONIC_TO_SEQ:
    typename = "GLFONT_GLYPH_CONIC_TO_SEQ";
    break;
  default:
    typename = "GLFONT_GLYPH_UNKNOWN";
    break;
  }
  const char *template = "<point type='%s' x=%ld y=%ld/>";

  sprintf(buff, template, typename, point->x, point->y);
}
