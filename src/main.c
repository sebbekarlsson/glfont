#include <glfont/glfont.h>
#include <glfont/macros.h>
#include <glfont/string_utils.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *filepath) {
  FILE *fp = fopen(filepath, "r");
  char *buffer = 0;
  size_t len;
  ssize_t bytes_read = getdelim(&buffer, &len, '\0', fp);
  return buffer;
}

static uint8_t *read_file_bytes(const char *filepath, uint32_t *buffer_len) {
  FILE *fp = fopen(filepath, "rb");
  if (!fp)
    printf("Error reading %s\n", filepath);
  uint8_t *buffer = 0;
  uint32_t len = 0;
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  *buffer_len = len;
  buffer = (uint8_t *)calloc(len, sizeof(uint8_t));
  fseek(fp, 0, SEEK_SET);
  fread(buffer, sizeof(uint8_t), len, fp);
  return buffer;
}

int main(int argc, char *argv[]) {
  uint32_t len = 0;
  uint8_t *bytes = read_file_bytes("../assets/font/Roboto-Regular.ttf", &len);
  GLFontFamily *family = glfont_init_font_family(bytes, len);
  GLFontGlyph *gl_glyph = NEW(GLFontGlyph);
  glfont_load_glyph(gl_glyph, family, 'A', (GLFontTextOptions){14});

  for (uint32_t i = 0; i < gl_glyph->points_len; i++) {
    GLFontGlyphPoint *point = gl_glyph->points[i];
    char buff[512];
    glfont_glyph_point_to_string(buff, point);
    printf("%s\n", buff);
  }

  glfont_glyph_free(gl_glyph);

  return 0;
}
