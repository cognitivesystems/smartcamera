The *GL Image Processing Framework* is a collection of low-level image
processing algorithms implemented using OpenGL ES 2.0-based GPGPU
techniques, along with utilities for acquiring, displaying, and saving
images. This page briefly introduces GLIPF's major components, and links
to their detailed documentation.

[Build instructions](build-instructions.md) and
[cross-compilation information](cross-compilation.md) can be found on
separate pages.

### Frame Sources ###

Frame sources provide image data to be processed. The framework supports
2 types of frame sources: video files and cameras. The former are useful
for testing, the latter are needed in real-world scenarios.

### Image Processors ###

Image processors are at the core of the framework: they implement
low-level image processing algorithms. They accept data returned by
frame sources as input, and output high-level information extracted from
it.

### Data Sinks ###

Data sinks are responsible for sharing data produced by the framework
with the outside world. To achieve that goal, they may publish the data
on a network, display or save it.
