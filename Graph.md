# Render graph notes

These are my notes for trying to implement some kind of render graph.

I want to be able to declare a bunch of passes, tell the renderer what depends on what, and have it run automatically.

## Passes

I am envisioning three kinds of passes:

* `RasterPass` will do basic rasterization
* `ComputePass` will dispatch a compute shader
* `FullscreenPass` will render a tri over the full screen in order to run a fragment shader.

Some inputs and outputs will be required. `static` inputs are set up in the constructor for the pass and do not change (their contents may change, but they should stay pointing to the same object). `dynamic` inputs come from 

### `RasterPass`

* Inputs
    * A shader program containing at least a vertex shader
    * Any number of uniforms, UBOs or SSBOs
        * Image inputs should be bindless!
    * Geometry (in the form of a VAO)
    * Draw commands (for now using `glMultiDrawElementsIndirect`)
    
* Outputs
    * A framebuffer

### `ComputePass`

* Inputs
    * `static` A compute shader
    * Work group size
    * Any number of buffers or textures
* Outputs
    * Anything


## TODO
* Reduce reliance on OpenGL specific concepts
    * VAO abstraction
    * "draw commands" abstraction
    * Standardize buffers (no ambiguity between UBO and SSBO)
* Consider an abstraction for textures / images to allow async loading / LOD management / GPU residency management.
* Ideally, same for models etc.