enum struct CursorMode : int {
    Normal = GLFW_CURSOR_NORMAL,
    Hidden = GLFW_CURSOR_HIDDEN,
    Disabled = GLFW_CURSOR_DISABLED,
    Captured = GLFW_CURSOR_CAPTURED
};

namespace WindowHint
{
    constexpr auto Focused = GLFW_FOCUSED;
    constexpr auto Iconified = GLFW_ICONIFIED;
    constexpr auto Resizable = GLFW_RESIZABLE;
    constexpr auto Visible = GLFW_VISIBLE;
    constexpr auto Decorated = GLFW_DECORATED;
    constexpr auto AutoIconify = GLFW_AUTO_ICONIFY;
    constexpr auto Floating = GLFW_FLOATING;
    constexpr auto Maximized = GLFW_MAXIMIZED;
    constexpr auto CenterCursor = GLFW_CENTER_CURSOR;
    constexpr auto TransparentFramebuffer = GLFW_TRANSPARENT_FRAMEBUFFER;
    constexpr auto Hovered = GLFW_HOVERED;
    constexpr auto FocusOnShow = GLFW_FOCUS_ON_SHOW;
    constexpr auto MousePassthrough = GLFW_MOUSE_PASSTHROUGH;
    constexpr auto PositionX = GLFW_POSITION_X;
    constexpr auto PositionY = GLFW_POSITION_Y;
    constexpr auto RedBits = GLFW_RED_BITS;
    constexpr auto GreenBits = GLFW_GREEN_BITS;
    constexpr auto BlueBits = GLFW_BLUE_BITS;
    constexpr auto AlphaBits = GLFW_ALPHA_BITS;
    constexpr auto DepthBits = GLFW_DEPTH_BITS;
    constexpr auto StencilBits = GLFW_STENCIL_BITS;
    constexpr auto AccumRedBits = GLFW_ACCUM_RED_BITS;
    constexpr auto AccumGreenBits = GLFW_ACCUM_GREEN_BITS;
    constexpr auto AccumBlueBits = GLFW_ACCUM_BLUE_BITS;
    constexpr auto AccumAlphaBits = GLFW_ACCUM_ALPHA_BITS;
    constexpr auto AuxBuffers = GLFW_AUX_BUFFERS;
    constexpr auto Stereo = GLFW_STEREO;
    constexpr auto Samples = GLFW_SAMPLES;
    constexpr auto SrgbCapable = GLFW_SRGB_CAPABLE;
    constexpr auto RefreshRate = GLFW_REFRESH_RATE;
    constexpr auto Doublebuffer = GLFW_DOUBLEBUFFER;
    constexpr auto ClientApi = GLFW_CLIENT_API;
    constexpr auto ContextVersionMajor = GLFW_CONTEXT_VERSION_MAJOR;
    constexpr auto ContextVersionMinor = GLFW_CONTEXT_VERSION_MINOR;
    constexpr auto ContextRevision = GLFW_CONTEXT_REVISION;
    constexpr auto ContextRobustness = GLFW_CONTEXT_ROBUSTNESS;
    constexpr auto OpenglForwardCompat = GLFW_OPENGL_FORWARD_COMPAT;
    constexpr auto ContextDebug = GLFW_CONTEXT_DEBUG;
    constexpr auto OpenglDebugContext = GLFW_OPENGL_DEBUG_CONTEXT;
    constexpr auto OpenglProfile = GLFW_OPENGL_PROFILE;
    constexpr auto ContextReleaseBehavior = GLFW_CONTEXT_RELEASE_BEHAVIOR;
    constexpr auto ContextNoError = GLFW_CONTEXT_NO_ERROR;
    constexpr auto ContextCreationApi = GLFW_CONTEXT_CREATION_API;
    constexpr auto ScaleToMonitor = GLFW_SCALE_TO_MONITOR;
    constexpr auto ScaleFramebuffer = GLFW_SCALE_FRAMEBUFFER;
    constexpr auto CocoaRetinaFramebuffer = GLFW_COCOA_RETINA_FRAMEBUFFER;
    constexpr auto CocoaFrameName = GLFW_COCOA_FRAME_NAME;
    constexpr auto CocoaGraphicsSwitching = GLFW_COCOA_GRAPHICS_SWITCHING;
    constexpr auto X11ClassName = GLFW_X11_CLASS_NAME;
    constexpr auto X11InstanceName = GLFW_X11_INSTANCE_NAME;
    constexpr auto Win32KeyboardMenu = GLFW_WIN32_KEYBOARD_MENU;
    constexpr auto Win32Showdefault = GLFW_WIN32_SHOWDEFAULT;
    constexpr auto WaylandAppId = GLFW_WAYLAND_APP_ID;

    constexpr auto NoApi = GLFW_NO_API;
    constexpr auto OpenglApi = GLFW_OPENGL_API;
    constexpr auto OpenglEsApi = GLFW_OPENGL_ES_API;
    constexpr auto NoRobustness = GLFW_NO_ROBUSTNESS;
    constexpr auto NoResetNotification = GLFW_NO_RESET_NOTIFICATION;
    constexpr auto LoseContextOnReset = GLFW_LOSE_CONTEXT_ON_RESET;
    constexpr auto OpenglAnyProfile = GLFW_OPENGL_ANY_PROFILE;
    constexpr auto OpenglCoreProfile = GLFW_OPENGL_CORE_PROFILE;
    constexpr auto OpenglCompatProfile = GLFW_OPENGL_COMPAT_PROFILE;
    constexpr auto Cursor = GLFW_CURSOR;
    constexpr auto StickyKeys = GLFW_STICKY_KEYS;
    constexpr auto StickyMouseButtons = GLFW_STICKY_MOUSE_BUTTONS;
    constexpr auto LockKeyMods = GLFW_LOCK_KEY_MODS;
    constexpr auto RawMouseMotion = GLFW_RAW_MOUSE_MOTION;
    constexpr auto CursorNormal = GLFW_CURSOR_NORMAL;
    constexpr auto CursorHidden = GLFW_CURSOR_HIDDEN;
    constexpr auto CursorDisabled = GLFW_CURSOR_DISABLED;
    constexpr auto CursorCaptured = GLFW_CURSOR_CAPTURED;
    constexpr auto AnyReleaseBehavior = GLFW_ANY_RELEASE_BEHAVIOR;
    constexpr auto ReleaseBehaviorFlush = GLFW_RELEASE_BEHAVIOR_FLUSH;
    constexpr auto ReleaseBehaviorNone = GLFW_RELEASE_BEHAVIOR_NONE;
    constexpr auto NativeContextApi = GLFW_NATIVE_CONTEXT_API;
    constexpr auto EglContextApi = GLFW_EGL_CONTEXT_API;
    constexpr auto OsmesaContextApi = GLFW_OSMESA_CONTEXT_API;
    constexpr auto AnglePlatformTypeNone = GLFW_ANGLE_PLATFORM_TYPE_NONE;
    constexpr auto AnglePlatformTypeOpengl = GLFW_ANGLE_PLATFORM_TYPE_OPENGL;
    constexpr auto AnglePlatformTypeOpengles = GLFW_ANGLE_PLATFORM_TYPE_OPENGLES;
    constexpr auto AnglePlatformTypeD3d9 = GLFW_ANGLE_PLATFORM_TYPE_D3D9;
    constexpr auto AnglePlatformTypeD3d11 = GLFW_ANGLE_PLATFORM_TYPE_D3D11;
    constexpr auto AnglePlatformTypeVulkan = GLFW_ANGLE_PLATFORM_TYPE_VULKAN;
    constexpr auto AnglePlatformTypeMetal = GLFW_ANGLE_PLATFORM_TYPE_METAL;
    constexpr auto WaylandPreferLibdecor = GLFW_WAYLAND_PREFER_LIBDECOR;
    constexpr auto WaylandDisableLibdecor = GLFW_WAYLAND_DISABLE_LIBDECOR;
    constexpr auto AnyPosition = GLFW_ANY_POSITION;
}

enum struct Key : uint16_t
{
    Space = GLFW_KEY_SPACE,
    Apostrophe = GLFW_KEY_APOSTROPHE,
    Comma = GLFW_KEY_COMMA,
    Minus = GLFW_KEY_MINUS,
    Period = GLFW_KEY_PERIOD,
    Slash = GLFW_KEY_SLASH,
    Key0 = GLFW_KEY_0,
    Key1 = GLFW_KEY_1,
    Key2 = GLFW_KEY_2,
    Key3 = GLFW_KEY_3,
    Key4 = GLFW_KEY_4,
    Key5 = GLFW_KEY_5,
    Key6 = GLFW_KEY_6,
    Key7 = GLFW_KEY_7,
    Key8 = GLFW_KEY_8,
    Key9 = GLFW_KEY_9,
    Semicolon = GLFW_KEY_SEMICOLON,
    Equal = GLFW_KEY_EQUAL,
    A = GLFW_KEY_A,
    B = GLFW_KEY_B,
    C = GLFW_KEY_C,
    D = GLFW_KEY_D,
    E = GLFW_KEY_E,
    F = GLFW_KEY_F,
    G = GLFW_KEY_G,
    H = GLFW_KEY_H,
    I = GLFW_KEY_I,
    J = GLFW_KEY_J,
    K = GLFW_KEY_K,
    L = GLFW_KEY_L,
    M = GLFW_KEY_M,
    N = GLFW_KEY_N,
    O = GLFW_KEY_O,
    P = GLFW_KEY_P,
    Q = GLFW_KEY_Q,
    R = GLFW_KEY_R,
    S = GLFW_KEY_S,
    T = GLFW_KEY_T,
    U = GLFW_KEY_U,
    V = GLFW_KEY_V,
    W = GLFW_KEY_W,
    X = GLFW_KEY_X,
    Y = GLFW_KEY_Y,
    Z = GLFW_KEY_Z,
    LeftBracket = GLFW_KEY_LEFT_BRACKET,
    Backslash = GLFW_KEY_BACKSLASH,
    RightBracket = GLFW_KEY_RIGHT_BRACKET,
    GraveAccent = GLFW_KEY_GRAVE_ACCENT,
    World1 = GLFW_KEY_WORLD_1,
    World2 = GLFW_KEY_WORLD_2,
    Escape = GLFW_KEY_ESCAPE,
    Enter = GLFW_KEY_ENTER,
    Tab = GLFW_KEY_TAB,
    Backspace = GLFW_KEY_BACKSPACE,
    Insert = GLFW_KEY_INSERT,
    Delete = GLFW_KEY_DELETE,
    Right = GLFW_KEY_RIGHT,
    Left = GLFW_KEY_LEFT,
    Down = GLFW_KEY_DOWN,
    Up = GLFW_KEY_UP,
    PageUp = GLFW_KEY_PAGE_UP,
    PageDown = GLFW_KEY_PAGE_DOWN,
    Home = GLFW_KEY_HOME,
    End = GLFW_KEY_END,
    CapsLock = GLFW_KEY_CAPS_LOCK,
    ScrollLock = GLFW_KEY_SCROLL_LOCK,
    NumLock = GLFW_KEY_NUM_LOCK,
    PrintScreen = GLFW_KEY_PRINT_SCREEN,
    Pause = GLFW_KEY_PAUSE,
    F1 = GLFW_KEY_F1,
    F2 = GLFW_KEY_F2,
    F3 = GLFW_KEY_F3,
    F4 = GLFW_KEY_F4,
    F5 = GLFW_KEY_F5,
    F6 = GLFW_KEY_F6,
    F7 = GLFW_KEY_F7,
    F8 = GLFW_KEY_F8,
    F9 = GLFW_KEY_F9,
    F10 = GLFW_KEY_F10,
    F11 = GLFW_KEY_F11,
    F12 = GLFW_KEY_F12,
    F13 = GLFW_KEY_F13,
    F14 = GLFW_KEY_F14,
    F15 = GLFW_KEY_F15,
    F16 = GLFW_KEY_F16,
    F17 = GLFW_KEY_F17,
    F18 = GLFW_KEY_F18,
    F19 = GLFW_KEY_F19,
    F20 = GLFW_KEY_F20,
    F21 = GLFW_KEY_F21,
    F22 = GLFW_KEY_F22,
    F23 = GLFW_KEY_F23,
    F24 = GLFW_KEY_F24,
    F25 = GLFW_KEY_F25,
    Kp0 = GLFW_KEY_KP_0,
    Kp1 = GLFW_KEY_KP_1,
    Kp2 = GLFW_KEY_KP_2,
    Kp3 = GLFW_KEY_KP_3,
    Kp4 = GLFW_KEY_KP_4,
    Kp5 = GLFW_KEY_KP_5,
    Kp6 = GLFW_KEY_KP_6,
    Kp7 = GLFW_KEY_KP_7,
    Kp8 = GLFW_KEY_KP_8,
    Kp9 = GLFW_KEY_KP_9,
    KpDecimal = GLFW_KEY_KP_DECIMAL,
    KpDivide = GLFW_KEY_KP_DIVIDE,
    KpMultiply = GLFW_KEY_KP_MULTIPLY,
    KpSubtract = GLFW_KEY_KP_SUBTRACT,
    KpAdd = GLFW_KEY_KP_ADD,
    KpEnter = GLFW_KEY_KP_ENTER,
    KpEqual = GLFW_KEY_KP_EQUAL,
    LeftShift = GLFW_KEY_LEFT_SHIFT,
    LeftControl = GLFW_KEY_LEFT_CONTROL,
    LeftAlt = GLFW_KEY_LEFT_ALT,
    LeftSuper = GLFW_KEY_LEFT_SUPER,
    RightShift = GLFW_KEY_RIGHT_SHIFT,
    RightControl = GLFW_KEY_RIGHT_CONTROL,
    RightAlt = GLFW_KEY_RIGHT_ALT,
    RightSuper = GLFW_KEY_RIGHT_SUPER,
    Menu = GLFW_KEY_MENU,
    Last = GLFW_KEY_LAST,
};

enum struct KeyAction : uint8_t
{
    Release = GLFW_RELEASE,
    Press = GLFW_PRESS,
    Repeat = GLFW_REPEAT,
};

enum struct KeyModifier : uint8_t
{
    None = 0,
    Shift = GLFW_MOD_SHIFT,
    Control = GLFW_MOD_CONTROL,
    Alt = GLFW_MOD_ALT,
    Super = GLFW_MOD_SUPER,
    CapsLock = GLFW_MOD_CAPS_LOCK,
    NumLock = GLFW_MOD_NUM_LOCK,
};

enum struct MouseButton : uint8_t
{
    Left,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8
};
