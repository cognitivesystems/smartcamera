#include <glipf/sources/v4l2-camera.h>

#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

#include <cstring>
#include <iostream>


namespace glipf {
namespace sources {


static void xioctl(int fh, int request, void* arg) {
  int result;

  do {
    result = v4l2_ioctl(fh, request, arg);
  } while (result == -1 && ((errno == EINTR) || (errno == EAGAIN)));

  if (result == -1)
    throw FrameSourceInitializationError("Error " + std::to_string(errno) +
                                         ", " + strerror(errno));
}


V4L2Camera::V4L2Camera(std::pair<size_t, size_t> imgDimensions,
                       std::string deviceName)
  : mCameraFD(-1)
  , mFrameProperties(imgDimensions, ColorSpace::BGR)
{
  mCameraFD = v4l2_open(deviceName.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (mCameraFD < 0)
    throw FrameSourceInitializationError("Could not open " + deviceName);

  struct v4l2_format fmt = v4l2_format();
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = mFrameProperties.dimensions().first;
  fmt.fmt.pix.height = mFrameProperties.dimensions().second;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
  xioctl(mCameraFD, VIDIOC_S_FMT, &fmt);

  if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_BGR24)
    throw FrameSourceInitializationError(
        "V4L2 driver doesn't accept BGR24 pixel format");

  if ((fmt.fmt.pix.width != mFrameProperties.dimensions().first) ||
      (fmt.fmt.pix.height != mFrameProperties.dimensions().second))
  {
    std::cerr << "Warning: driver is sending image at " << fmt.fmt.pix.width
              << "x" << fmt.fmt.pix.height << '\n';
    mFrameProperties =
        FrameProperties(std::make_pair(fmt.fmt.pix.width, fmt.fmt.pix.height),
                        mFrameProperties.colorSpace());
  }

  struct v4l2_requestbuffers req = v4l2_requestbuffers();
  req.count = 2;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  xioctl(mCameraFD, VIDIOC_REQBUFS, &req);

  mBuffers.resize(req.count);

  for (size_t n_buffers = 0; n_buffers < req.count; ++n_buffers) {
    struct v4l2_buffer buf = v4l2_buffer();

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers;

    xioctl(mCameraFD, VIDIOC_QUERYBUF, &buf);
    void* bufAddr = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                              MAP_SHARED, mCameraFD, buf.m.offset);

    mBuffers[n_buffers].first = static_cast<uint8_t*>(bufAddr);
    mBuffers[n_buffers].second = buf.length;

    if (MAP_FAILED == mBuffers[n_buffers].first)
      throw FrameSourceInitializationError("Failed to mmap V4L2 buffers");
  }

  mUnmappedBufferIndex = 0;

  for (size_t i = 1; i < req.count; ++i) {
    struct v4l2_buffer buf = v4l2_buffer();
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    xioctl(mCameraFD, VIDIOC_QBUF, &buf);
  }

  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(mCameraFD, VIDIOC_STREAMON, &type);
}


V4L2Camera::~V4L2Camera() {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(mCameraFD, VIDIOC_STREAMOFF, &type);

  for (const auto& bufferData : mBuffers)
    v4l2_munmap(bufferData.first, bufferData.second);

  v4l2_close(mCameraFD);
}


const FrameProperties& V4L2Camera::getFrameProperties() const {
  return mFrameProperties;
}


const uint8_t* V4L2Camera::grabFrame() {
  if (mUnmappedBufferIndex) {
    struct v4l2_buffer buf = v4l2_buffer();
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = *mUnmappedBufferIndex;
    mUnmappedBufferIndex = boost::none;
    xioctl(mCameraFD, VIDIOC_QBUF, &buf);
  }

  fd_set fds;
  int result;

  do {
    FD_ZERO(&fds);
    FD_SET(mCameraFD, &fds);

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    result = select(mCameraFD + 1, &fds, NULL, NULL, &timeout);
  } while ((result == -1 && (errno = EINTR)));

  if (result == -1) {
    std::cerr << "Frame acquisition timed out\n";
    return nullptr;
  }

  struct v4l2_buffer buf = v4l2_buffer();
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  xioctl(mCameraFD, VIDIOC_DQBUF, &buf);
  mUnmappedBufferIndex = buf.index;

  return mBuffers[buf.index].first;
}


} // end namespace sources
} // end namespace glipf
