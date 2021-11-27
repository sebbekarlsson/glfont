#version 430 core

out vec4 FragColor;
uniform vec4 color;

in vec2 texcoord;
in vec4 colors;
in vec3 tex_offset;

uniform sampler3D imgtexture;

void main()
{
  vec4 sampled = vec4(1.0, 1.0, 1.0, texture(imgtexture, vec3(texcoord.x + tex_offset.x, texcoord.y + tex_offset.y, tex_offset.z)).r);
  FragColor = (color * sampled);
}
