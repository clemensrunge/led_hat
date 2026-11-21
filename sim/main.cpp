// LED hat simulator: runs LED_hat.ino on the host and draws the NeoPixel matrix
// wrapped around a cylinder (or flat, for debugging). The sketch's two UARTs
// are available as terminals in an ImGui console.
//
// Geometry is projected on the CPU and drawn with SDL_RenderGeometry. The hat
// is a closed convex solid, so back-face culling is all the hidden-surface
// removal it needs.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "sim.h"

// Matrix size, from the sketch's PixelWidth and PixelHeight (see sketch.cpp).
extern const int SketchCols, SketchRows;
static const int Cols = SketchCols;
static const int Rows = SketchRows;

static const float Pi = 3.14159265358979f;

struct Vec3
{
  float x, y, z;
};
static Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
static float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 normalize(Vec3 a) { return a * (1.0f / std::sqrt(dot(a, a))); }

// Same serpentine wiring as xyToNum() in the sketch: even rows run left to
// right, odd rows right to left.
static int pixelIndex(int x, int y)
{
  return (y & 1) ? (y + 1) * Cols - 1 - x : y * Cols + x;
}

// Raw NeoPixel values look far too dark on a monitor next to the real thing
// (the eye's response to LEDs is strongly nonlinear), so lift them.
static SDL_FColor ledColor(uint32_t c, float scale = 1.0f)
{
  auto lift = [](uint32_t v) { return std::pow(v / 255.0f, 0.4f); };
  return {lift((c >> 16) & 0xFF) * scale, lift((c >> 8) & 0xFF) * scale, lift(c & 0xFF) * scale, 1.0f};
}

static SDL_FColor mix(SDL_FColor a, SDL_FColor b, float t)
{
  return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

class Camera
{
public:
  float yaw = 0, pitch = 0.3f, dist = 8.0f;
  float cx = 0, cy = 0, focal = 1;

  void setViewport(float w, float h)
  {
    cx = w * 0.5f;
    cy = h * 0.5f;
    focal = h / (2.0f * std::tan(30.0f * Pi / 360.0f));  // 30 degree vertical FOV
  }

  // World -> view rotation. The eye sits at (0, 0, dist) looking down -z.
  Vec3 rotate(Vec3 p) const
  {
    float x1 = std::cos(yaw) * p.x - std::sin(yaw) * p.z;
    float z1 = std::sin(yaw) * p.x + std::cos(yaw) * p.z;
    float y2 = std::cos(pitch) * p.y - std::sin(pitch) * z1;
    float z2 = std::sin(pitch) * p.y + std::cos(pitch) * z1;
    return {x1, y2, z2};
  }

  SDL_FPoint project(Vec3 v) const
  {
    float depth = dist - v.z;
    return {cx + focal * v.x / depth, cy - focal * v.y / depth};
  }

  // How directly a surface at view-space point v with normal n faces the eye (<= 0: facing away).
  float facing(Vec3 v, Vec3 n) const { return dot(n, normalize(Vec3{0, 0, dist} - v)); }
};

class Batch
{
public:
  void clear()
  {
    verts.clear();
    idx.clear();
  }
  int vertex(SDL_FPoint p, SDL_FColor c)
  {
    verts.push_back({p, c, {0, 0}});
    return (int)verts.size() - 1;
  }
  void tri(int a, int b, int c) { idx.insert(idx.end(), {a, b, c}); }
  void draw(SDL_Renderer *r, SDL_BlendMode mode)
  {
    if (idx.empty()) return;
    SDL_SetRenderDrawBlendMode(r, mode);
    SDL_RenderGeometry(r, nullptr, verts.data(), (int)verts.size(), idx.data(), (int)idx.size());
  }

  // Filled ellipse spanned by center + a*cos + b*sin, fading from inner to outer color.
  void disc(SDL_FPoint c, SDL_FPoint a, SDL_FPoint b, SDL_FColor inner, SDL_FColor outer, int segments = 14)
  {
    int center = vertex(c, inner);
    int first = (int)verts.size();
    for (int i = 0; i < segments; i++)
    {
      float t = 2 * Pi * i / segments;
      vertex({c.x + a.x * std::cos(t) + b.x * std::sin(t), c.y + a.y * std::cos(t) + b.y * std::sin(t)}, outer);
    }
    for (int i = 0; i < segments; i++) tri(center, first + i, first + (i + 1) % segments);
  }

private:
  std::vector<SDL_Vertex> verts;
  std::vector<int> idx;
};

class HatRenderer
{
public:
  float arcDegrees = 360;  // how much of the circumference the columns cover

  void draw(SDL_Renderer *r, const Camera &cam, const std::vector<uint32_t> &px)
  {
    const float radius = 1.0f;
    const float step = arcDegrees * Pi / 180.0f / Cols;  // angle per column
    const float pitch = radius * step;                  // LED spacing, same vertically
    const float bottom = -4.5f * pitch, top = 5.5f * pitch;
    const Vec3 light = normalize({-0.4f, 0.5f, 1.0f});  // view space, from upper left

    // Body: band of quads plus top and bottom caps.
    body.clear();
    auto shade = [&](Vec3 n) {
      float l = 0.25f + 0.75f * std::fmax(0.0f, dot(n, light));
      return SDL_FColor{0.13f * l, 0.13f * l, 0.17f * l, 1};
    };
    const int segs = Cols * 4;
    for (int i = 0; i < segs; i++)
    {
      float t0 = 2 * Pi * i / segs, t1 = 2 * Pi * (i + 1) / segs, tm = (t0 + t1) / 2;
      Vec3 nm = cam.rotate({std::sin(tm), 0, std::cos(tm)});
      Vec3 mid = cam.rotate({radius * std::sin(tm), 0, radius * std::cos(tm)});
      if (cam.facing(mid, nm) <= 0) continue;
      Vec3 n0 = cam.rotate({std::sin(t0), 0, std::cos(t0)});
      Vec3 n1 = cam.rotate({std::sin(t1), 0, std::cos(t1)});
      int a = body.vertex(cam.project(cam.rotate({radius * std::sin(t0), bottom, radius * std::cos(t0)})), shade(n0));
      int b = body.vertex(cam.project(cam.rotate({radius * std::sin(t1), bottom, radius * std::cos(t1)})), shade(n1));
      int c = body.vertex(cam.project(cam.rotate({radius * std::sin(t1), top, radius * std::cos(t1)})), shade(n1));
      int d = body.vertex(cam.project(cam.rotate({radius * std::sin(t0), top, radius * std::cos(t0)})), shade(n0));
      body.tri(a, b, c);
      body.tri(a, c, d);
    }
    for (float y : {top, bottom})
    {
      Vec3 n = cam.rotate({0, y > 0 ? 1.0f : -1.0f, 0});
      Vec3 center = cam.rotate({0, y, 0});
      if (cam.facing(center, n) <= 0) continue;
      SDL_FColor col = shade(n);
      int c = body.vertex(cam.project(center), col);
      int first = c + 1;
      for (int i = 0; i < segs; i++)
      {
        float t = 2 * Pi * i / segs;
        body.vertex(cam.project(cam.rotate({radius * std::sin(t), y, radius * std::cos(t)})), col);
      }
      for (int i = 0; i < segs; i++) body.tri(c, first + i, first + (i + 1) % segs);
    }
    body.draw(r, SDL_BLENDMODE_NONE);

    // LEDs. Column 0 is on the left of the front, so text reads correctly
    // from outside; with a full 360 degree arc the seam is at the back.
    leds.clear();
    glow.clear();
    for (int y = 0; y < Rows; y++)
    {
      for (int x = 0; x < Cols; x++)
      {
        float theta = (x + 0.5f) * step - arcDegrees * Pi / 360.0f;
        float h = (3 - y) * pitch;  // row 0 at the top
        Vec3 n = {std::sin(theta), 0, std::cos(theta)};
        Vec3 tangent = {std::cos(theta), 0, -std::sin(theta)};
        Vec3 center = cam.rotate(Vec3{n.x * radius * 1.002f, h, n.z * radius * 1.002f});
        float face = cam.facing(center, cam.rotate(n));
        if (face <= 0) continue;

        // Project the LED's local axes so it foreshortens with the surface.
        auto axis = [&](Vec3 dir, float size) {
          SDL_FPoint p0 = cam.project(center);
          SDL_FPoint p1 = cam.project(center + cam.rotate(dir) * size);
          return SDL_FPoint{p1.x - p0.x, p1.y - p0.y};
        };
        SDL_FPoint c = cam.project(center);
        uint32_t v = px.size() > (size_t)pixelIndex(x, y) ? px[pixelIndex(x, y)] : 0;
        float edge = std::fmin(1.0f, face * 3.0f);  // soften toward the silhouette

        if (v == 0)
        {
          SDL_FColor off = {0.06f, 0.06f, 0.07f, 1};
          leds.disc(c, axis(tangent, 0.3f * pitch), axis({0, 1, 0}, 0.3f * pitch), off, off, 10);
          continue;
        }
        SDL_FColor col = ledColor(v, edge);
        SDL_FColor hot = mix(col, {edge, edge, edge, 1}, 0.45f);
        leds.disc(c, axis(tangent, 0.32f * pitch), axis({0, 1, 0}, 0.32f * pitch), hot, col);
        SDL_FColor g = ledColor(v, 0.55f * edge), none = {0, 0, 0, 0};
        glow.disc(c, axis(tangent, 1.1f * pitch), axis({0, 1, 0}, 1.1f * pitch), g, none, 18);
      }
    }
    leds.draw(r, SDL_BLENDMODE_NONE);
    glow.draw(r, SDL_BLENDMODE_ADD);
  }

  void drawFlat(SDL_Renderer *r, int w, int h, const std::vector<uint32_t> &px)
  {
    float cell = std::fmin(w / (Cols + 2.0f), h / (Rows + 4.0f));
    float ox = (w - cell * Cols) / 2, oy = (h - cell * Rows) / 2;
    leds.clear();
    glow.clear();
    for (int y = 0; y < Rows; y++)
    {
      for (int x = 0; x < Cols; x++)
      {
        SDL_FPoint c = {ox + (x + 0.5f) * cell, oy + (y + 0.5f) * cell};
        uint32_t v = px.size() > (size_t)pixelIndex(x, y) ? px[pixelIndex(x, y)] : 0;
        if (v == 0)
        {
          SDL_FColor off = {0.08f, 0.08f, 0.09f, 1};
          leds.disc(c, {0.3f * cell, 0}, {0, 0.3f * cell}, off, off, 10);
          continue;
        }
        SDL_FColor col = ledColor(v);
        leds.disc(c, {0.32f * cell, 0}, {0, 0.32f * cell}, mix(col, {1, 1, 1, 1}, 0.45f), col);
        glow.disc(c, {1.1f * cell, 0}, {0, 1.1f * cell}, ledColor(v, 0.55f), {0, 0, 0, 0}, 18);
      }
    }
    leds.draw(r, SDL_BLENDMODE_NONE);
    glow.draw(r, SDL_BLENDMODE_ADD);
  }

private:
  Batch body, leds, glow;
};

// A serial terminal per UART: scrollback of what the sketch printed, and a
// line input that sends the text plus '\n' (the menu reads up to LF).
class Console
{
public:
  void poll()
  {
    for (int p = 0; p < sim::PortCount; p++)
    {
      std::string out = sim::takeOutput((sim::Port)p);
      if (out.empty()) continue;
      for (char c : out)
        if (c != '\r') log[p] += c;
      if (log[p].size() > 200000) log[p].erase(0, log[p].size() - 150000);
      scrollDown[p] = true;
    }
  }

  void draw(float showRate)
  {
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos({vp->WorkPos.x + vp->WorkSize.x - 470, vp->WorkPos.y + 10}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({460, vp->WorkSize.y - 20}, ImGuiCond_FirstUseEver);
    ImGui::Begin("UART console");
    left = ImGui::GetWindowPos().x;

    if (sim::statusLed())
      ImGui::TextColored({1.0f, 0.67f, 0.16f, 1}, "status LED ON (menu open)");
    else
      ImGui::TextDisabled("status LED off");
    ImGui::SameLine();
    ImGui::TextDisabled("| show(): %.1f fps", showRate);

    if (ImGui::BeginTabBar("ports"))
    {
      static const char *names[sim::PortCount] = {"Serial (USB, 57600)", "Bluetooth (SoftS, 9600)"};
      for (int p = 0; p < sim::PortCount; p++)
      {
        if (!ImGui::BeginTabItem(names[p])) continue;
        terminal((sim::Port)p);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
  }

  float left = 1e9f;  // x of the console's left edge, so the 3D view can center beside it

private:
  void terminal(sim::Port p)
  {
    float inputHeight = ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("log", {0, -inputHeight}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(log[p].data(), log[p].data() + log[p].size());
    if (scrollDown[p] || ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
    scrollDown[p] = false;
    ImGui::EndChild();

    ImGui::SetNextItemWidth(-60);
    bool enter = ImGui::InputTextWithHint("##in", "Enter opens the menu", input[p], sizeof input[p],
                                          ImGuiInputTextFlags_EnterReturnsTrue);
    if (enter)
    {
      sim::send(p, std::string(input[p]) + "\n");
      input[p][0] = 0;
      ImGui::SetKeyboardFocusHere(-1);  // -1 = the input above; must come before the next widget
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) log[p].clear();
  }

  std::string log[sim::PortCount];
  char input[sim::PortCount][128] = {};
  bool scrollDown[sim::PortCount] = {};
};

static void usage(const char *argv0)
{
  std::printf("usage: %s [--arc DEGREES] [--screenshot FILE.bmp]\n"
              "  --arc         how much of the hat's circumference the columns cover (default 360)\n"
              "  --screenshot  save the window after 3 seconds and exit\n",
              argv0);
}

int main(int argc, char **argv)
{
  HatRenderer hat;
  const char *screenshot = nullptr;
  for (int i = 1; i < argc; i++)
  {
    if (!std::strcmp(argv[i], "--arc") && i + 1 < argc)
      hat.arcDegrees = std::fmax(10.0f, std::fmin(360.0f, (float)std::atof(argv[++i])));
    else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc)
      screenshot = argv[++i];
    else
    {
      usage(argv[0]);
      return !std::strcmp(argv[i], "--help") ? 0 : 1;
    }
  }

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window *window;
  SDL_Renderer *renderer;
  if (!SDL_CreateWindowAndRenderer("LED hat simulator", 1100, 650, SDL_WINDOW_RESIZABLE,
                                   &window, &renderer))
  {
    std::fprintf(stderr, "SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetRenderVSync(renderer, 1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  sim::start();
  Console console;

  Camera cam;
  bool flat = false, autoRotate = true, dragging = false;
  std::vector<uint32_t> px;
  uint64_t lastShows = 0, lastTick = SDL_GetTicks();
  float showRate = 0;
  uint64_t prevFrame = SDL_GetTicks();

  for (bool running = true; running;)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
      ImGui_ImplSDL3_ProcessEvent(&e);
      const ImGuiIO &io = ImGui::GetIO();
      if (io.WantCaptureKeyboard && (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP)) continue;
      if (io.WantCaptureMouse && (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_WHEEL)) continue;
      switch (e.type)
      {
      case SDL_EVENT_QUIT:
        running = false;
        break;
      case SDL_EVENT_KEY_DOWN:
        if (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_Q) running = false;
        if (e.key.key == SDLK_TAB) flat = !flat;
        if (e.key.key == SDLK_R) autoRotate = !autoRotate;
        if (e.key.key == SDLK_HOME) cam = Camera();
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e.button.button == SDL_BUTTON_LEFT) dragging = true, autoRotate = false;
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button == SDL_BUTTON_LEFT) dragging = false;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (dragging)
        {
          cam.yaw += e.motion.xrel * 0.008f;
          cam.pitch = std::fmax(-1.2f, std::fmin(1.2f, cam.pitch + e.motion.yrel * 0.008f));
        }
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        cam.dist = std::fmax(2.0f, std::fmin(15.0f, cam.dist * std::pow(0.9f, e.wheel.y)));
        break;
      }
    }

    uint64_t now = SDL_GetTicks();
    float dt = (now - prevFrame) / 1000.0f;
    prevFrame = now;
    if (autoRotate) cam.yaw += dt * 0.35f;

    uint64_t shows = sim::latestFrame(px);
    if (now - lastTick >= 1000)
    {
      showRate = (shows - lastShows) * 1000.0f / (now - lastTick);
      lastShows = shows;
      lastTick = now;
    }

    int w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    cam.setViewport(std::fmax(w * 0.4f, std::fmin((float)w, console.left)), h);

    SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
    SDL_RenderClear(renderer);
    if (flat)
      hat.drawFlat(renderer, w, h, px);
    else
      hat.draw(renderer, cam, px);

    console.poll();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    console.draw(showRate);
    ImGui::SetNextWindowPos({10, (float)h - 10}, ImGuiCond_Always, {0, 1});
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("help", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextDisabled("drag: orbit  wheel: zoom  R: auto-rotate  Tab: flat/3D  Home: reset view  Esc: quit");
    ImGui::End();
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    if (screenshot && SDL_GetTicks() > 3000)
    {
      SDL_Surface *shot = SDL_RenderReadPixels(renderer, nullptr);
      if (!shot || !SDL_SaveBMP(shot, screenshot)) std::fprintf(stderr, "screenshot failed: %s\n", SDL_GetError());
      SDL_DestroySurface(shot);
      running = false;
    }
    SDL_RenderPresent(renderer);
  }

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  // The sketch thread never returns (loop() runs forever); leave without
  // running static destructors underneath it.
  std::_Exit(0);
}
