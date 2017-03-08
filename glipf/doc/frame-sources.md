@namespace glipf::sources

Frame sources provide image data to be processed. The framework supports
2 types of frame sources: video files (OpenCvVideoSource) and cameras
(V4L2Camera and OpenCvCamera). The former are useful for testing, the
latter are needed in real-world scenarios.
