#pragma once
#include <GL/glew.h>
#include <memory>
#include "renderbuffer.h"
#include <unordered_map>

namespace game_engine {
namespace opengl {
struct renderbuffer_attachment {
  static renderbuffer_attachment color_attachmenti(size_t i);
  static renderbuffer_attachment depth_attachment;
  static renderbuffer_attachment stencil_attachment;
  static renderbuffer_attachment depth_stencil_attachment;
private:
  friend renderbuffer_attachment init_renderbuff_at(GLenum constant);
  friend class framebuffer;
  GLenum gl_constant;
};
class framebuffer {
public:
  enum class render_target : GLenum {
    read = GL_DRAW_FRAMEBUFFER,
    draw = GL_READ_FRAMEBUFFER,
    read_and_draw = GL_FRAMEBUFFER
  };

public:
  framebuffer();
  ~framebuffer();

  void bind_to(render_target target);

  /** \brief Attaches a passed renderbuffer to a specific attachment.
   * It takes a shared_ptr that is converted to a weak_ptr which is then
   * checked to see if no renderbuffer bound to an attachment of this
   * framebuffer has been destroyed. */
  void attach(renderbuffer_attachment rnd_at,
              const std::shared_ptr<const renderbuffer> &ptr);

  /** \brief Attaches a passed renderbuffer to a specific attachment.
   * It does not take a shared_ptr to let the user manage renderbuffers as they
   * see fit. */
  void attach(renderbuffer_attachment rnd_at,
              const renderbuffer& rbuffer);

private:
  /** \brief Returns true and stores the render_target this framebuffer is
   * currently bound to. If there is no such target, it returns false and
   * does not touch the contents of the passed reference.
   */
  bool get_bind(render_target &ret);
private:
  GLuint framebuffer_id;
  std::unordered_map<renderbuffer_attachment,
                     std::weak_ptr<const renderbuffer_attachment>
                    > rbuff_attachments;
};
}
}
