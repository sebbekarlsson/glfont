#include <glfont/glfont.h>
#include <glfont/macros.h>
#include <glfont/string_utils.h>
#include <stdlib.h>
#include <string.h>
#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

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

typedef struct {
  unsigned int id;
  unsigned int vertex_shader;
  unsigned int fragment_shader;
} ShaderProgram;

ShaderProgram compile_shader_program(const char *vertex_shader_text,
                                     const char *fragment_shader_text) {

  int success;
  char infoLog[512];
  unsigned int vertex_shader;
  unsigned int fragment_shader;
  unsigned int program;

  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, (const GLchar *const *)&vertex_shader_text,
                 NULL);
  glCompileShader(vertex_shader);

  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
    printf("Could not compile vertex_shader %s\n", infoLog);
    return (ShaderProgram){};
  }

  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1,
                 (const GLchar *const *)&fragment_shader_text, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
    printf("Could not compile vertex_shader %s\n", infoLog);
    return (ShaderProgram){};
  }

  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    printf("Error compiling shader program: %s\n", infoLog);
    return (ShaderProgram){};
  }

  return (ShaderProgram){program, vertex_shader, fragment_shader};
}

typedef enum { SHADER_VARIABLE_MAT4, SHADER_VARIABLE_VEC4 } EShaderVariableType;

typedef struct {
  EShaderVariableType type;
  const char* name;

  mat4 mat4_value;
  float* vec4_value;
} ShaderVariable;

typedef struct {
  ShaderProgram *program;
  unsigned int VBO;
  unsigned int EBO;
  ShaderVariable** variables;
  uint32_t variables_len;
} DrawObject;

typedef struct {
  unsigned int index;
  uint32_t size;
  GLenum type;
  GLboolean normalized;
  GLsizei stride;
  void *ptr;
} Attribute;



ShaderVariable SHADER_VAR(EShaderVariableType type, const char* name, mat4 mat4_value, float* vec4_value)  {
  ShaderVariable v = {};
  ShaderVariable* var = &v; //NEW(ShaderVariable);
  var->vec4_value = vec4_value;
  var->type = type;
  var->name = strdup(name);
  if (mat4_value) {
    glm_mat4_copy(mat4_value, var->mat4_value);
  }
  return v;
}

#define ATTRIBUTE(index, size, type, normalized, stride)                       \
  (Attribute) { index, size, type, normalized, stride, 0 }

DrawObject *draw(DrawObject *draw_object, const char *vertex_shader,
                 const char *fragment_shader, float vertices[],
                 uint32_t vertices_cols, uint32_t vertices_rows,
                 unsigned int indices[], uint32_t indices_len,
                 Attribute attributes[], uint32_t attributes_len, ShaderVariable variables[], uint32_t variables_len, unsigned int VAO) {
  uint32_t array_offset = 0;
  uint32_t element_offset = 0;
  unsigned int vertices_len = vertices_cols * vertices_rows;
  if (!draw_object->program) {
    ShaderProgram program =
        compile_shader_program(vertex_shader, fragment_shader);
    draw_object->program = NEW(ShaderProgram);
    draw_object->program->id = program.id;
    draw_object->program->vertex_shader = program.vertex_shader;
    draw_object->program->fragment_shader = program.fragment_shader;
    glGenBuffers(1, &draw_object->VBO);
    glGenBuffers(1, &draw_object->EBO);
  }

  glUseProgram(draw_object->program->id);

  if (vertices_len) {
    glBindBuffer(GL_ARRAY_BUFFER, draw_object->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices_len * sizeof(float), 0,
                 GL_STATIC_DRAW); // allocate
    glBufferSubData(GL_ARRAY_BUFFER, array_offset, vertices_len * sizeof(float),
                    vertices);
  }

  if (indices_len) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, draw_object->VBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertices_len * sizeof(float), 0,
                 GL_STATIC_DRAW); // allocate
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, array_offset,
                    indices_len * sizeof(unsigned int), indices);
  }

  uint32_t cursor = 0;
  for (uint32_t i = 0; i < attributes_len; i++) {
    Attribute attr = attributes[i];
    // x, y, z
    glVertexAttribPointer(attr.index, attr.size, attr.type, GL_FALSE,
                          vertices_cols * sizeof(float),
                          (void *)0 +
                              (array_offset + (cursor * sizeof(float))));
    glEnableVertexAttribArray(attr.index);
    cursor += attr.size;
  }

  for (uint32_t i = 0; i < variables_len; i++) {
    ShaderVariable* var = &variables[i];

    switch (var->type) {
      case SHADER_VARIABLE_MAT4: {

      glUniformMatrix4fv(glGetUniformLocation(draw_object->program->id, var->name), 1, GL_FALSE,
                       *var->mat4_value);
    }; break;
      case SHADER_VARIABLE_VEC4: {
        glUniform4fv(glGetUniformLocation(draw_object->program->id, var->name), 1, (float[4]){ var->vec4_value[0], var->vec4_value[1], var->vec4_value[2], var->vec4_value[3] });
      }; break;
      default: { /*noop*/ }; break;
    }
  }


  glBindVertexArray(VAO);
  glDrawArrays(GL_LINES, 0, 2);

  return draw_object;
}

int main(int argc, char *argv[]) {

  if (!glfwInit()) {
    printf("glwInit error\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_FLOATING, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL App", NULL, NULL);
  if (!window) {
    printf("Could not create window");
    return 1;
  }

  glfwMakeContextCurrent(window);

  GLenum err = glewInit();

  if (GLEW_OK != err) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    //  return 1;
  }

  unsigned int VAO;
  unsigned int vertex_shader;
  unsigned int fragment_shader;

  char *vertex_shader_text = read_file("../assets/shaders/glfont_vertex.glsl");
  char *fragment_shader_text = read_file("../assets/shaders/glfont_frag.glsl");

  if (!vertex_shader_text) {
    printf("Could not load vertex shader\n");
    return 1;
  }

  if (!fragment_shader_text) {
    printf("Could not load vertex shader\n");
    return 1;
  }

  mat4 view = GLM_MAT4_IDENTITY_INIT;
  mat4 projection = GLM_MAT4_IDENTITY_INIT;
  mat4 model = GLM_MAT4_IDENTITY_INIT;

  glm_ortho(0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, -100.0f,
            100.0f, projection);

  glm_translate(view, (vec3){0.0f, 0.0f, 0.0f});
  glm_translate(model, (vec3){0.0f, 0.0f, 0.0f});

  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  ShaderProgram program =
      compile_shader_program(vertex_shader_text, fragment_shader_text);

  glUseProgram(program.id);

  glUniformMatrix4fv(glGetUniformLocation(program.id, "view"), 1, GL_FALSE,
                     *view);
  glUniformMatrix4fv(glGetUniformLocation(program.id, "projection"), 1,
                     GL_FALSE, *projection);
  glUniformMatrix4fv(glGetUniformLocation(program.id, "model"), 1, GL_FALSE,
                     *model);

  float r = 0;
  float g = 0;
  float b = 0;
  float a = 1.0f;
  GLFontAtlas *atlas = 0;
  uint32_t len = 0;
  uint8_t *bytes = read_file_bytes("../assets/font/Roboto-Regular.ttf", &len);
  GLFontFamily *family = glfont_init_font_family(bytes, len);
  DrawObject draw_object = {};
   float* lineColor = (float*)calloc(4, sizeof(float));
    lineColor[0] = 1.0f;
    lineColor[1] = 1.0f;
    lineColor[2] = 1.0f;
    lineColor[3] = 1.0f;
  while (!glfwWindowShouldClose(window)) {
    glBindVertexArray(VAO);
    glUseProgram(program.id);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(r / 255, g / 255, b / 255, a);

    GLFontColor color =
        GLFONT_COLOR(255 / 255.0f, 255 / 255.0f, 255 / 255.0f, 1.0f);

    glUniformMatrix4fv(glGetUniformLocation(program.id, "view"), 1, GL_FALSE,
                       *view);
    glUniformMatrix4fv(glGetUniformLocation(program.id, "projection"), 1,
                       GL_FALSE, *projection);
    glUniformMatrix4fv(glGetUniformLocation(program.id, "model"), 1, GL_FALSE,
                       *model);


    draw(&draw_object,
         "#version 440 core\n"
         "layout (location = 0) in vec3 aPos;\n"
         "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
         "void main()\n"
         "{\n"
         "gl_Position = projection * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
         "   //gl_Position = MVP * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
         "}\0",
         "#version 440 core\n"
         "out vec4 FragColor;\n"
         "uniform vec4 color;\n"
         "void main()\n"
         "{\n"
         "   FragColor = color;\n"
         "}\n\0",
         (float[]){0.0f, 0.0f, 0.0f, 1920.0f, 1080.0f, 0.0f}, 3, 2, 0, 0,
         (Attribute[]){

             ATTRIBUTE(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float))

         },
         1, draw_object.variables_len ? 0 : (ShaderVariable[]){
         SHADER_VAR(SHADER_VARIABLE_VEC4, "color", 0, lineColor),
         SHADER_VAR(SHADER_VARIABLE_MAT4, "projection", projection, 0),
         SHADER_VAR(SHADER_VARIABLE_MAT4, "view", view, 0),
         SHADER_VAR(SHADER_VARIABLE_MAT4, "model", model, 0),

       }, 4, VAO);


    //    glfont_draw_text_instanced(
    //       atlas, "hello world", 0, 0, 0,
    //       (GLFontTextOptions){
    //          32, color, 1, (GLFontAtlasOptions){family->bytes, family->len,
    //          32}},
    //     view, projection, program.id, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  /*
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

    glfont_glyph_free(gl_glyph);*/

  return 0;
}
