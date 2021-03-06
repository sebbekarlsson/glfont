# glfont (WIP)
> optimized OpenGL .ttf glyph renderer.  
> **work in progress**

## The optimization
> The optimization works like this:
1. Generate a 3-Dimensional texture atlas containing all glyphs  
   This atlas has the width of the widest character, and the height of the
   highest character.  
   Each character exists in it's own z-index.
   Where the z-index is: `ascii_code - 32`.
   
2. Slice the text to be rendered into chunks.  
   Each chunk has the size of `256` bytes.  
   The reason for it to be exactly `256` is because it seems to be  
   the maximum length of a static vec3 array on the GPU at most vendors.
   
   The plan is to dynamically buffer data to the graphics card so that we don't
   have to do any chunking though.
   
   
3. Send each chunk where each character is a quad, to the graphics card.  
   Other information about the character is sent here as well.
   Such as the width, height, color, etc.
   
4. perform instanced rendering.

## Prerequisites
> This library does **not** do the following:
* Create a window for you
* Create an OpenGL context for you
* Compile shaders for you
* Free any memory for you

## Shaders
> There are some shader's [here](assets/shaders) that are supposed to be put to use
> when rendering text using this library.  
> However, as mentioned before: You have to compile the shaders on your own.
> They just exist here for you to copy-paste them.

## Usage
```C
/**
  The recommended use here is to pass in `0` to the `atlas` argument.
  This function will then create one for you and return it.
  Then it's up to you to free it.
  
  
  When the `dynamic` parameter is set to 1,
  then this function will automatically re-generate buffers for you
  when the text changes.
  Passing `0` to the `dynamic` parameter makes things faster however.
**/
GLFontAtlas *glfont_draw_text_instanced(GLFontAtlas *atlas, const char *text,
                                        float x, float y, float z,
                                        GLFontTextOptions options, mat4 view,
                                        mat4 projection, unsigned int program,
                                        unsigned int dynamic);
                                        
                                        
// the file you should include
#include <glfont/glfont.h>
```

