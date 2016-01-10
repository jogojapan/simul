#version 120

// Can be shared between shaders, e.g. fs and vs
varying vec2 texCoord0;

// Variable that is writable for CPU and readable for GPU
uniform sampler2D diffuse;

void main()
{
  // gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  gl_FragColor = texture2D(diffuse,texCoord0);
}
