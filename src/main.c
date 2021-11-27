#include <glfont/glfont.h>
#include <glfont/string_utils.h>
#include <string.h>

static char *read_file(const char *filepath) {
  FILE *fp = fopen(filepath, "r");
  char *buffer = NULL;
  size_t len;
  ssize_t bytes_read = getdelim(&buffer, &len, '\0', fp);
  return buffer;
}

int main(int argc, char *argv[]) {
  char *contents = read_file("../assets/text/sample.txt");
  uint32_t len = 0;
  char **chunks = glfont_str_chunk(contents, 256, &len);

  for (int i = 0; i < len; i++) {
    printf("CHUNK(%ld)(%s)\n", strlen(chunks[i]), chunks[i]);
  }
  return 0;
}
