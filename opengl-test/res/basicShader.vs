#version 120

attribute vec3 position;
attribute vec2 texCoord;

varying vec2 texCoord0;

// A variable that can be set by the CPU (i.e. from the main program)
uniform mat4 transform;

void main()
{
  gl_Position = transform * vec4(position, 1.0);
  texCoord0   = texCoord;
}
