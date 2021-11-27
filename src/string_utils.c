#include <glfont/string_utils.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

char **glfont_str_chunk(const char *src, uint32_t chunk_length, uint32_t *len) {
  if (!src)
    return 0;
  uint32_t input_len = strlen(src);
  if (!input_len)
    return 0;

  chunk_length = input_len % chunk_length;
  uint32_t nr_chunks = ceil(input_len / chunk_length);

  if (nr_chunks == 0) {
    nr_chunks = 1;
    chunk_length = input_len;
  }

  *len = nr_chunks;

  char **chunks = (char **)calloc(nr_chunks, sizeof(char *));

  uint32_t size_copied = 0;
  for (int i = 0; i < nr_chunks; i++) {
    uint32_t size_left = input_len - size_copied;
    uint32_t size_to_copy = MIN(size_left, chunk_length);

    chunks[i] = (char *)calloc(size_to_copy + 1, sizeof(char));
    memcpy(chunks[i], &src[i * size_to_copy], chunk_length);
    size_copied += (chunk_length + 1);
  }

  return chunks;
}

uint32_t glfont_str_count_alpha(const char *src) {
  uint32_t len = strlen(src);

  uint32_t c = 0;
  for (uint32_t i = 0; i < len; i++) {
    char ch = src[i];
    c += (int)(ch != '\r' && ch != '\n' && ch != ' ');
  }

  return c;
}

uint32_t glfont_str_next_break(const char *src, uint32_t start) {
  if (!src)
    return 0;
  uint32_t len = strlen(src);
  if (len < start)
    return 0;

  for (uint32_t i = start; i < len; i++) {
    char ch = src[i];
    if (ch == '\r' || ch == '\n') {
      return i;
    }
  }

  return 0;
}
