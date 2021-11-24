#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColors;
layout (location = 2) in vec2 aTexCoord;

out vec2 texcoord;
out vec4 colors;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec4 color;

uniform vec3 offsets[100];
uniform vec3 toff[100];
out vec3 tex_offset;



void main()
{
  vec3 offset = offsets[gl_InstanceID];
  vec3 texture_offset = toff[gl_InstanceID];
  gl_Position = projection * view * model * vec4(aPos + offset, 1.0);
  texcoord = aTexCoord;
  colors = aColors;
  texcoord = aTexCoord;
  tex_offset = texture_offset;
}
