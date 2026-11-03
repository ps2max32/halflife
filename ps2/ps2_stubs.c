/* ps2_stubs.c: what the PS2 port leaves out, as calls that do nothing.

   The OpenGL calls ref_gl links to (XASH_GL_STATIC) that libgl does not
   have: framebuffer objects, 1D/3D, multisample and compressed textures,
   debug output (all behind extensions libgl does not report, so not
   called), and fog, texture coordinate generation, normals and the front
   face (called; nothing yet: libgl has no fog).

   The POSIX calls the engine links to that the PS2 has no use for: there is
   one program and one thread. */

#include <errno.h>
#include <time.h>

/* ---- OpenGL ------------------------------------------------------------------- */

#define GL_NOTHING(name) \
  void name(void) {}

GL_NOTHING(glFogf)
GL_NOTHING(glFogfv)
GL_NOTHING(glFogi)
GL_NOTHING(glTexGeni)
GL_NOTHING(glNormal3fv)
GL_NOTHING(glFrontFace)
GL_NOTHING(glTexCoord3f)
GL_NOTHING(glTexImage1D)
GL_NOTHING(glTexSubImage1D)
GL_NOTHING(glTexImage3D)
GL_NOTHING(glTexSubImage3D)
GL_NOTHING(glTexImage2DMultisample)
GL_NOTHING(glCompressedTexImage1DARB)
GL_NOTHING(glCompressedTexImage3DARB)
GL_NOTHING(glCompressedTexSubImage1DARB)
GL_NOTHING(glCompressedTexSubImage2DARB)
GL_NOTHING(glCompressedTexSubImage3DARB)
GL_NOTHING(glGenFramebuffers)
GL_NOTHING(glDeleteFramebuffers)
GL_NOTHING(glBindFramebuffer)
GL_NOTHING(glFramebufferTexture2D)
GL_NOTHING(glFramebufferRenderbuffer)
GL_NOTHING(glBlitFramebuffer)
GL_NOTHING(glGenRenderbuffers)
GL_NOTHING(glDeleteRenderbuffers)
GL_NOTHING(glBindRenderbuffer)
GL_NOTHING(glRenderbufferStorage)
GL_NOTHING(glRenderbufferStorageMultisample)
GL_NOTHING(glDebugMessageControlARB)
GL_NOTHING(glDebugMessageCallbackARB)

/* not complete: GL_FRAMEBUFFER_COMPLETE is 0x8CD5 */
unsigned glCheckFramebufferStatus(unsigned target) { return 0; }

/* ---- POSIX -------------------------------------------------------------------- */

int nanosleep(const struct timespec *t, struct timespec *rem) { return 0; }
int fsync(int fd) { return 0; }
int execv(const char *path, char *const argv[]) {
  errno = ENOSYS;
  return -1;
}
int execvp(const char *file, char *const argv[]) {
  errno = ENOSYS;
  return -1;
}
