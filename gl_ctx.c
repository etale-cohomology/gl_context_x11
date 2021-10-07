/*
gcc  gl_ctx.c -o gl_ctx  -lX11 -lX11-xcb -lxcb -lGL  &&  ./gl_ctx

xdpyinfo
glxinfo

X Window System
X
X11
x.org
xorg

x11 display
x11 screen

xcb connection
xcb screen
xcb colormap
xcb window

glx fbconfig
glx context
glx window
*/
#include <X11/X.h>
#include <X11/Xlib-xcb.h>

#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <GL/glxext.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

xcb_screen_t* xcb_get_screen(xcb_connection_t* connection, int screen_idx){
  xcb_screen_iterator_t it = xcb_setup_roots_iterator (xcb_get_setup (connection));
  for (; it.rem; --screen_idx, xcb_screen_next (&it))
    if (screen_idx == 0)
      return it.data;
  return NULL;
}

void glx_fbconfig_meta(Display* xlib_display, GLXFBConfig glx_fbconfig){
  int fbconfig_id;             glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_FBCONFIG_ID,    &fbconfig_id);
  int fbconfig_visual;         glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_VISUAL_ID,      &fbconfig_visual);

  int fbconfig_doublebuffer;   glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_DOUBLEBUFFER,   &fbconfig_doublebuffer);
  int fbconfig_sample_buffers; glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_SAMPLE_BUFFERS, &fbconfig_sample_buffers);
  int fbconfig_samples;        glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_SAMPLES,        &fbconfig_samples);
  int fbconfig_stereo;         glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_STEREO,         &fbconfig_stereo);
  int fbconfig_aux_buffers;    glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_AUX_BUFFERS,    &fbconfig_aux_buffers);

  int fbconfig_red_size;       glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_RED_SIZE,       &fbconfig_red_size);
  int fbconfig_green_size;     glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_GREEN_SIZE,     &fbconfig_green_size);
  int fbconfig_blue_size;      glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_BLUE_SIZE,      &fbconfig_blue_size);
  int fbconfig_alpha_size;     glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_ALPHA_SIZE,     &fbconfig_alpha_size);

  int fbconfig_buffer_size;    glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_BUFFER_SIZE,    &fbconfig_buffer_size);
  int fbconfig_depth_size;     glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_DEPTH_SIZE,     &fbconfig_depth_size);
  int fbconfig_stencil_size;   glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_STENCIL_SIZE,   &fbconfig_stencil_size);

  printf("  \x1b[94mGLXFBConfig  \x1b[31m%03x \x1b[32m%2x  \x1b[31m%d \x1b[32m%d \x1b[94m%2d \x1b[37m%d \x1b[37m%d  \x1b[31m%d \x1b[32m%d \x1b[94m%d \x1b[35m%d  \x1b[31m%2d \x1b[32m%2d \x1b[94m%1d\x1b[0m\n", fbconfig_id,fbconfig_visual, fbconfig_doublebuffer,fbconfig_sample_buffers,fbconfig_samples,fbconfig_stereo,fbconfig_aux_buffers, fbconfig_red_size,fbconfig_green_size,fbconfig_blue_size,fbconfig_alpha_size, fbconfig_buffer_size,fbconfig_depth_size,fbconfig_stencil_size);
}

#include <poll.h>
xcb_generic_event_t* xcb_ev_poll(xcb_connection_t* connection, int timeout_ms){  // `xcb_generic_event_t` is a polymorphic data structure! The first 8 bits tell you how to cast it, and depending on how you cast it, the interpretation of its binary layout (which is fixed in width) changes!
  struct pollfd pfd = {0x00};
  pfd.events        = POLLIN;  // POLLIN: there's data to read!
  pfd.fd            = xcb_get_file_descriptor(connection);
  int ntriggers     = poll(&pfd, 1, timeout_ms);  // WARN! We CANNOT wait for ntriggers! Otherwise we'll wait processing on events and the screen will go blank because glViewport() will not trigger! Hard to explain, but it happens to me!
  return xcb_poll_for_event(connection);  // printf("ntriggers %d  ev_event %016lu\n", ntriggers, (u64)ev_event);
}

int main(){
  Display* xlib_display   = XOpenDisplay(":0");
  int      x11_screen_idx = DefaultScreen(xlib_display);

  XSetEventQueueOwner(xlib_display, XCBOwnsEventQueue);
  xcb_connection_t* xcb_connection = XGetXCBConnection(xlib_display);
  xcb_screen_t*     xcb_screen     = xcb_get_screen(xcb_connection, x11_screen_idx);


  int glx_fbconfig_attrs[] = {
    GLX_BUFFER_SIZE,   16,
    GLX_DOUBLEBUFFER,  0,
    GLX_SAMPLES,       0,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    0,
  };
  int glx_nfbconfigs;
  GLXFBConfig* glx_fbconfigs = glXChooseFBConfig(xlib_display, x11_screen_idx, glx_fbconfig_attrs, &glx_nfbconfigs);  if(glx_nfbconfigs==0) puts("\x1b[91mFAIL\x1b[0m");
  GLXFBConfig  glx_fbconfig  = glx_fbconfigs[0];
  // for(int i=0; i<glx_nfbconfigs; ++i)
    // glx_fbconfig_meta(xlib_display, glx_fbconfigs[i]);
  XFree(glx_fbconfigs);
  int glx_visual_id; glXGetFBConfigAttrib(xlib_display, glx_fbconfig, GLX_VISUAL_ID, &glx_visual_id);

  uint32_t xcb_colormap = xcb_generate_id(xcb_connection);
  xcb_create_colormap(xcb_connection, XCB_COLORMAP_ALLOC_NONE, xcb_colormap, xcb_screen->root, glx_visual_id);

  // typedef enum xcb_cw_t {
  //     XCB_CW_BACK_PIXMAP = 1,
  // /**< Overrides the default background-pixmap. The background pixmap and window must
  // have the same root and same depth. Any size pixmap can be used, although some
  // sizes may be faster than others.

  // If `XCB_BACK_PIXMAP_NONE` is specified, the window has no defined background.
  // The server may fill the contents with the previous screen contents or with
  // contents of its own choosing.

  // If `XCB_BACK_PIXMAP_PARENT_RELATIVE` is specified, the parent's background is
  // used, but the window must have the same depth as the parent (or a Match error
  // results).   The parent's background is tracked, and the current version is
  // used each time the window background is required. */

  //     XCB_CW_BACK_PIXEL = 2,
  // /**< Overrides `BackPixmap`. A pixmap of undefined size filled with the specified
  // background pixel is used for the background. Range-checking is not performed,
  // the background pixel is truncated to the appropriate number of bits. */

  //     XCB_CW_BORDER_PIXMAP = 4,
  // /**< Overrides the default border-pixmap. The border pixmap and window must have the
  // same root and the same depth. Any size pixmap can be used, although some sizes
  // may be faster than others.

  // The special value `XCB_COPY_FROM_PARENT` means the parent's border pixmap is
  // copied (subsequent changes to the parent's border attribute do not affect the
  // child), but the window must have the same depth as the parent. */

  //     XCB_CW_BORDER_PIXEL = 8,
  // /**< Overrides `BorderPixmap`. A pixmap of undefined size filled with the specified
  // border pixel is used for the border. Range checking is not performed on the
  // border-pixel value, it is truncated to the appropriate number of bits. */

  //     XCB_CW_BIT_GRAVITY = 16,
  // /**< Defines which region of the window should be retained if the window is resized. */

  //     XCB_CW_WIN_GRAVITY = 32,
  // /**< Defines how the window should be repositioned if the parent is resized (see
  // `ConfigureWindow`). */

  //     XCB_CW_BACKING_STORE = 64,
  // /**< A backing-store of `WhenMapped` advises the server that maintaining contents of
  // obscured regions when the window is mapped would be beneficial. A backing-store
  // of `Always` advises the server that maintaining contents even when the window
  // is unmapped would be beneficial. In this case, the server may generate an
  // exposure event when the window is created. A value of `NotUseful` advises the
  // server that maintaining contents is unnecessary, although a server may still
  // choose to maintain contents while the window is mapped. Note that if the server
  // maintains contents, then the server should maintain complete contents not just
  // the region within the parent boundaries, even if the window is larger than its
  // parent. While the server maintains contents, exposure events will not normally
  // be generated, but the server may stop maintaining contents at any time. */

  //     XCB_CW_BACKING_PLANES = 128,
  // /**< The backing-planes indicates (with bits set to 1) which bit planes of the
  // window hold dynamic data that must be preserved in backing-stores and during
  // save-unders. */

  //     XCB_CW_BACKING_PIXEL = 256,
  // /**< The backing-pixel specifies what value to use in planes not covered by
  // backing-planes. The server is free to save only the specified bit planes in the
  // backing-store or save-under and regenerate the remaining planes with the
  // specified pixel value. Any bits beyond the specified depth of the window in
  // these values are simply ignored. */

  //     XCB_CW_OVERRIDE_REDIRECT = 512,
  // /**< The override-redirect specifies whether map and configure requests on this
  // window should override a SubstructureRedirect on the parent, typically to
  // inform a window manager not to tamper with the window. */

  //     XCB_CW_SAVE_UNDER = 1024,
  // /**< If 1, the server is advised that when this window is mapped, saving the
  // contents of windows it obscures would be beneficial. */

  //     XCB_CW_EVENT_MASK = 2048,
  // /**< The event-mask defines which events the client is interested in for this window
  // (or for some event types, inferiors of the window). */

  //     XCB_CW_DONT_PROPAGATE = 4096,
  // /**< The do-not-propagate-mask defines which events should not be propagated to
  // ancestor windows when no client has the event type selected in this window. */

  //     XCB_CW_COLORMAP = 8192,
  // /**< The colormap specifies the colormap that best reflects the true colors of the window. Servers
  // capable of supporting multiple hardware colormaps may use this information, and window man-
  // agers may use it for InstallColormap requests. The colormap must have the same visual type
  // and root as the window (or a Match error results). If CopyFromParent is specified, the parent's
  // colormap is copied (subsequent changes to the parent's colormap attribute do not affect the child).
  // However, the window must have the same visual type as the parent (or a Match error results),
  // and the parent must not have a colormap of None (or a Match error results). For an explanation
  // of None, see FreeColormap request. The colormap is copied by sharing the colormap object
  // between the child and the parent, not by making a complete copy of the colormap contents. */

  //     XCB_CW_CURSOR = 16384
  // /**< If a cursor is specified, it will be used whenever the pointer is in the window. If None is speci-
  // fied, the parent's cursor will be used when the pointer is in the window, and any change in the
  // parent's cursor will cause an immediate change in the displayed cursor. */

  // } xcb_cw_t;
  // typedef enum xcb_event_mask_t {
  //     XCB_EVENT_MASK_NO_EVENT = 0,
  //     XCB_EVENT_MASK_KEY_PRESS = 1,
  //     XCB_EVENT_MASK_KEY_RELEASE = 2,
  //     XCB_EVENT_MASK_BUTTON_PRESS = 4,
  //     XCB_EVENT_MASK_BUTTON_RELEASE = 8,
  //     XCB_EVENT_MASK_ENTER_WINDOW = 16,
  //     XCB_EVENT_MASK_LEAVE_WINDOW = 32,
  //     XCB_EVENT_MASK_POINTER_MOTION = 64,
  //     XCB_EVENT_MASK_POINTER_MOTION_HINT = 128,
  //     XCB_EVENT_MASK_BUTTON_1_MOTION = 256,
  //     XCB_EVENT_MASK_BUTTON_2_MOTION = 512,
  //     XCB_EVENT_MASK_BUTTON_3_MOTION = 1024,
  //     XCB_EVENT_MASK_BUTTON_4_MOTION = 2048,
  //     XCB_EVENT_MASK_BUTTON_5_MOTION = 4096,
  //     XCB_EVENT_MASK_BUTTON_MOTION = 8192,
  //     XCB_EVENT_MASK_KEYMAP_STATE = 16384,
  //     XCB_EVENT_MASK_EXPOSURE = 32768,
  //     XCB_EVENT_MASK_VISIBILITY_CHANGE = 65536,
  //     XCB_EVENT_MASK_STRUCTURE_NOTIFY = 131072,
  //     XCB_EVENT_MASK_RESIZE_REDIRECT = 262144,
  //     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY = 524288,
  //     XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT = 1048576,
  //     XCB_EVENT_MASK_FOCUS_CHANGE = 2097152,
  //     XCB_EVENT_MASK_PROPERTY_CHANGE = 4194304,
  //     XCB_EVENT_MASK_COLOR_MAP_CHANGE = 8388608,
  //     XCB_EVENT_MASK_OWNER_GRAB_BUTTON = 16777216
  // } xcb_event_mask_t;

  uint32_t value_mask   = XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  uint32_t value_list[] = {XCB_BACK_PIXMAP_NONE, XCB_EVENT_MASK_KEY_PRESS|XCB_EVENT_MASK_KEY_RELEASE|XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY, xcb_colormap};
  uint32_t xcb_window   = xcb_generate_id(xcb_connection);
  xcb_create_window(xcb_connection, xcb_screen->root_depth, xcb_window,xcb_screen->root, 0,0,xcb_screen->width_in_pixels,xcb_screen->height_in_pixels,0, XCB_WINDOW_CLASS_INPUT_OUTPUT,glx_visual_id, value_mask,value_list);
  xcb_map_window(xcb_connection, xcb_window);
  xcb_flush(xcb_connection);
  printf("%d  \x1b[31m%d \x1b[32m%d \x1b[94m%d  \x1b[0m%d  %08x\n", x11_screen_idx, xcb_screen->width_in_pixels,xcb_screen->height_in_pixels,xcb_screen->root_depth, glx_nfbconfigs, xcb_colormap);

  int glx_context_attrs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB,   4,
    GLX_CONTEXT_MINOR_VERSION_ARB,   6,
    GLX_CONTEXT_FLAGS_ARB,           GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    GLX_CONTEXT_PROFILE_MASK_ARB,    GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    GLX_CONTEXT_OPENGL_NO_ERROR_ARB, 1,  // GLX_ARB_create_context_no_error
    0,
  };

  GLXWindow  glx_window  = glXCreateWindow(xlib_display, glx_fbconfig, xcb_window, NULL);
  GLXContext glx_context = glXCreateContextAttribsARB(xlib_display, glx_fbconfig, NULL, 1, glx_context_attrs);
  glXMakeContextCurrent(xlib_display, glx_window,glx_window, glx_context);

  // ----------------------------------------------------------------------------------------------------------------------------#
  glClearColor(0.0,0.6,1.0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glFlush();

  int running = 1;
  while(running){
    xcb_generic_event_t* xcb_ev = xcb_ev_poll(xcb_connection, 16);
    if(xcb_ev!=NULL){
      switch(xcb_ev->response_type & 0b01111111){
        case XCB_KEY_PRESS:{
          xcb_key_press_event_t* xcb_key_press = (xcb_key_press_event_t*)xcb_ev;
          xcb_keycode_t          xcb_keycode = xcb_key_press->detail;
          switch(xcb_keycode){
            default: running = 0; break;
          }
        }break;
      }
    free(xcb_ev);
    }
  }

  // ----------------------------------------------------------------------------------------------------------------------------#
  xcb_destroy_window(xcb_connection, xcb_window);
  xcb_free_colormap( xcb_connection, xcb_colormap);

  glXDestroyWindow(xlib_display,  glx_window);
  glXDestroyContext(xlib_display, glx_context);

  XCloseDisplay(xlib_display);
}
