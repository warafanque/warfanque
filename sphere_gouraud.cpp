#define UNICODE
#define _UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cmath>
#include <vector>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

// Constants
const double PI = 3.14159265358979323846;
const int SPHERE_SEGMENTS_LAT = 30;  // Latitude divisions
const int SPHERE_SEGMENTS_LON = 40;  // Longitude divisions
const double SPHERE_RADIUS = 200.0;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitializeGraphics(HWND hwnd);
void CleanupGraphics();
void RenderScene(HDC hdc, int width, int height);
void UpdateRotation();

// 3D Vector structure
struct Vector3 {
    double x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(double s) const { return Vector3(x * s, y * s, z * s); }
    
    double dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    
    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    double length() const { return sqrt(x * x + y * y + z * z); }
    
    Vector3 normalize() const {
        double len = length();
        if (len > 0.0001) return Vector3(x / len, y / len, z / len);
        return *this;
    }
};

// Vertex structure
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector3 transformedPos;
    Vector3 transformedNormal;
    COLORREF color;
    int screenX, screenY;
    double screenZ;
};

// Triangle structure
struct Triangle {
    int v0, v1, v2;
};

// Global variables
HWND g_hwndMain = NULL;
HWND g_hwndToolbar = NULL;
std::vector<Vertex> g_vertices;
std::vector<Triangle> g_triangles;
std::vector<double> g_zBuffer;
int g_zBufferWidth = 0;
int g_zBufferHeight = 0;

// Rotation angles
double g_rotationX = 0.0;
double g_rotationY = 0.0;
double g_rotationZ = 0.0;

// Animation state
bool g_isAnimating = false;
UINT_PTR g_timerID = 0;

// Light source position (upper right)
Vector3 g_lightPos(300.0, 400.0, 500.0);

// Material properties
Vector3 g_ambientColor(0.2, 0.2, 0.2);
Vector3 g_diffuseColor(0.8, 0.6, 0.4);
Vector3 g_specularColor(1.0, 1.0, 1.0);
double g_shininess = 32.0;

// Light properties
Vector3 g_lightAmbient(0.3, 0.3, 0.3);
Vector3 g_lightDiffuse(1.0, 1.0, 1.0);
Vector3 g_lightSpecular(1.0, 1.0, 1.0);

// Toolbar button IDs
#define ID_TOOLBAR_ANIMATE 1001

// Matrix operations
void RotateX(Vector3& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    double y = v.y * c - v.z * s;
    double z = v.y * s + v.z * c;
    v.y = y;
    v.z = z;
}

void RotateY(Vector3& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    double x = v.x * c + v.z * s;
    double z = -v.x * s + v.z * c;
    v.x = x;
    v.z = z;
}

void RotateZ(Vector3& v, double angle) {
    double c = cos(angle);
    double s = sin(angle);
    double x = v.x * c - v.y * s;
    double y = v.x * s + v.y * c;
    v.x = x;
    v.y = y;
}

// Create sphere mesh using geographic subdivision
void CreateSphereMesh() {
    g_vertices.clear();
    g_triangles.clear();
    
    // Create vertices
    for (int lat = 0; lat <= SPHERE_SEGMENTS_LAT; lat++) {
        double theta = PI * lat / SPHERE_SEGMENTS_LAT;  // 0 to PI
        double sinTheta = sin(theta);
        double cosTheta = cos(theta);
        
        for (int lon = 0; lon <= SPHERE_SEGMENTS_LON; lon++) {
            double phi = 2.0 * PI * lon / SPHERE_SEGMENTS_LON;  // 0 to 2*PI
            double sinPhi = sin(phi);
            double cosPhi = cos(phi);
            
            Vertex vertex;
            // Position
            vertex.position.x = SPHERE_RADIUS * sinTheta * cosPhi;
            vertex.position.y = SPHERE_RADIUS * cosTheta;
            vertex.position.z = SPHERE_RADIUS * sinTheta * sinPhi;
            
            // Normal (for sphere, normal is normalized position)
            vertex.normal = vertex.position.normalize();
            
            g_vertices.push_back(vertex);
        }
    }
    
    // Create triangles
    for (int lat = 0; lat < SPHERE_SEGMENTS_LAT; lat++) {
        for (int lon = 0; lon < SPHERE_SEGMENTS_LON; lon++) {
            int first = lat * (SPHERE_SEGMENTS_LON + 1) + lon;
            int second = first + SPHERE_SEGMENTS_LON + 1;
            
            // First triangle
            Triangle tri1;
            tri1.v0 = first;
            tri1.v1 = second;
            tri1.v2 = first + 1;
            g_triangles.push_back(tri1);
            
            // Second triangle
            Triangle tri2;
            tri2.v0 = second;
            tri2.v1 = second + 1;
            tri2.v2 = first + 1;
            g_triangles.push_back(tri2);
        }
    }
}

// Calculate Gouraud shading color for a vertex
COLORREF CalculateVertexColor(const Vertex& vertex) {
    // Light direction
    Vector3 lightDir = (g_lightPos - vertex.transformedPos).normalize();
    
    // View direction (assuming camera at origin looking along -Z)
    Vector3 viewDir = Vector3(0, 0, 1);
    
    // Ambient component
    Vector3 ambient = Vector3(
        g_ambientColor.x * g_lightAmbient.x,
        g_ambientColor.y * g_lightAmbient.y,
        g_ambientColor.z * g_lightAmbient.z
    );
    
    // Diffuse component
    double diff = std::max(0.0, vertex.transformedNormal.dot(lightDir));
    Vector3 diffuse = Vector3(
        g_diffuseColor.x * g_lightDiffuse.x * diff,
        g_diffuseColor.y * g_lightDiffuse.y * diff,
        g_diffuseColor.z * g_lightDiffuse.z * diff
    );
    
    // Specular component (Blinn-Phong)
    Vector3 halfDir = (lightDir + viewDir).normalize();
    double spec = pow(std::max(0.0, vertex.transformedNormal.dot(halfDir)), g_shininess);
    Vector3 specular = Vector3(
        g_specularColor.x * g_lightSpecular.x * spec,
        g_specularColor.y * g_lightSpecular.y * spec,
        g_specularColor.z * g_lightSpecular.z * spec
    );
    
    // Combine components
    Vector3 color = ambient + diffuse + specular;
    
    // Clamp to [0, 1] and convert to COLORREF
    int r = std::min(255, std::max(0, (int)(color.x * 255)));
    int g = std::min(255, std::max(0, (int)(color.y * 255)));
    int b = std::min(255, std::max(0, (int)(color.z * 255)));
    
    return RGB(r, g, b);
}

// Transform and project vertices
void TransformVertices(int width, int height) {
    int centerX = width / 2;
    int centerY = height / 2;
    
    for (size_t i = 0; i < g_vertices.size(); i++) {
        Vertex& v = g_vertices[i];
        
        // Copy original position and normal
        v.transformedPos = v.position;
        v.transformedNormal = v.normal;
        
        // Apply rotations
        RotateX(v.transformedPos, g_rotationX);
        RotateY(v.transformedPos, g_rotationY);
        RotateZ(v.transformedPos, g_rotationZ);
        
        RotateX(v.transformedNormal, g_rotationX);
        RotateY(v.transformedNormal, g_rotationY);
        RotateZ(v.transformedNormal, g_rotationZ);
        v.transformedNormal = v.transformedNormal.normalize();
        
        // Calculate vertex color (Gouraud shading)
        v.color = CalculateVertexColor(v);
        
        // Project to screen (orthographic projection)
        v.screenX = centerX + (int)v.transformedPos.x;
        v.screenY = centerY - (int)v.transformedPos.y;  // Invert Y for screen coordinates
        v.screenZ = v.transformedPos.z;
    }
}

// Interpolate color
COLORREF InterpolateColor(COLORREF c1, COLORREF c2, COLORREF c3, double w1, double w2, double w3) {
    int r = (int)(GetRValue(c1) * w1 + GetRValue(c2) * w2 + GetRValue(c3) * w3);
    int g = (int)(GetGValue(c1) * w1 + GetGValue(c2) * w2 + GetGValue(c3) * w3);
    int b = (int)(GetBValue(c1) * w1 + GetBValue(c2) * w2 + GetBValue(c3) * w3);
    
    r = std::min(255, std::max(0, r));
    g = std::min(255, std::max(0, g));
    b = std::min(255, std::max(0, b));
    
    return RGB(r, g, b);
}

// Barycentric coordinates
void GetBarycentricCoords(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3,
                          double& w1, double& w2, double& w3) {
    double denom = (double)((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
    if (fabs(denom) < 0.0001) {
        w1 = w2 = w3 = 0;
        return;
    }
    
    w1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / denom;
    w2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / denom;
    w3 = 1.0 - w1 - w2;
}

// Draw filled triangle with Gouraud shading and Z-buffering
void DrawTriangleGouraud(HDC hdc, const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    // Backface culling
    Vector3 edge1 = v1.transformedPos - v0.transformedPos;
    Vector3 edge2 = v2.transformedPos - v0.transformedPos;
    Vector3 normal = edge1.cross(edge2);
    
    // View direction (camera looking along -Z axis)
    Vector3 viewDir(0, 0, 1);
    if (normal.dot(viewDir) <= 0) return;  // Back face
    
    // Bounding box
    int minX = std::min(v0.screenX, std::min(v1.screenX, v2.screenX));
    int maxX = std::max(v0.screenX, std::max(v1.screenX, v2.screenX));
    int minY = std::min(v0.screenY, std::min(v1.screenY, v2.screenY));
    int maxY = std::max(v0.screenY, std::max(v1.screenY, v2.screenY));
    
    // Clamp to z-buffer bounds
    minX = std::max(0, minX);
    maxX = std::min(g_zBufferWidth - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(g_zBufferHeight - 1, maxY);
    
    // Rasterize
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            double w0, w1, w2;
            GetBarycentricCoords(x, y, v0.screenX, v0.screenY, v1.screenX, v1.screenY,
                               v2.screenX, v2.screenY, w0, w1, w2);
            
            // Check if point is inside triangle
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                // Interpolate Z
                double z = v0.screenZ * w0 + v1.screenZ * w1 + v2.screenZ * w2;
                
                // Z-buffer test
                int bufferIndex = y * g_zBufferWidth + x;
                if (z > g_zBuffer[bufferIndex]) {
                    g_zBuffer[bufferIndex] = z;
                    
                    // Interpolate color (Gouraud shading)
                    COLORREF color = InterpolateColor(v0.color, v1.color, v2.color, w0, w1, w2);
                    SetPixel(hdc, x, y, color);
                }
            }
        }
    }
}

// Render the scene
void RenderScene(HDC hdc, int width, int height) {
    // Clear Z-buffer
    if (g_zBufferWidth != width || g_zBufferHeight != height) {
        g_zBufferWidth = width;
        g_zBufferHeight = height;
        g_zBuffer.resize(width * height);
    }
    std::fill(g_zBuffer.begin(), g_zBuffer.end(), -10000.0);
    
    // Transform vertices
    TransformVertices(width, height);
    
    // Draw triangles
    for (const Triangle& tri : g_triangles) {
        const Vertex& v0 = g_vertices[tri.v0];
        const Vertex& v1 = g_vertices[tri.v1];
        const Vertex& v2 = g_vertices[tri.v2];
        
        DrawTriangleGouraud(hdc, v0, v1, v2);
    }
}

// Initialize graphics
void InitializeGraphics(HWND hwnd) {
    CreateSphereMesh();
}

// Cleanup graphics
void CleanupGraphics() {
    g_vertices.clear();
    g_triangles.clear();
    g_zBuffer.clear();
}

// Update rotation for animation
void UpdateRotation() {
    g_rotationY += 0.02;
    if (g_rotationY > 2.0 * PI) g_rotationY -= 2.0 * PI;
}

// Toggle animation
void ToggleAnimation(HWND hwnd) {
    g_isAnimating = !g_isAnimating;
    
    if (g_isAnimating) {
        g_timerID = SetTimer(hwnd, 1, 16, NULL);  // ~60 FPS
    } else {
        if (g_timerID != 0) {
            KillTimer(hwnd, g_timerID);
            g_timerID = 0;
        }
    }
}

// Create toolbar
HWND CreateToolBar(HWND hwndParent) {
    HWND hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0,
        hwndParent, (HMENU)IDC_TOOLBAR, GetModuleHandle(NULL), NULL);
    
    if (hwndToolbar) {
        SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
        
        TBBUTTON tbb[1];
        ZeroMemory(tbb, sizeof(tbb));
        
        tbb[0].iBitmap = I_IMAGENONE;
        tbb[0].idCommand = ID_TOOLBAR_ANIMATE;
        tbb[0].fsState = TBSTATE_ENABLED;
        tbb[0].fsStyle = TBSTYLE_BUTTON;
        tbb[0].iString = (INT_PTR)L"动画";
        
        SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)1, (LPARAM)&tbb);
        SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0);
    }
    
    return hwndToolbar;
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        InitCommonControls();
        g_hwndToolbar = CreateToolBar(hwnd);
        InitializeGraphics(hwnd);
        return 0;
        
    case WM_DESTROY:
        if (g_timerID != 0) {
            KillTimer(hwnd, g_timerID);
        }
        CleanupGraphics();
        PostQuitMessage(0);
        return 0;
        
    case WM_SIZE:
        if (g_hwndToolbar) {
            SendMessage(g_hwndToolbar, TB_AUTOSIZE, 0, 0);
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
        
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        RECT rect;
        GetClientRect(hwnd, &rect);
        
        // Get toolbar height
        int toolbarHeight = 0;
        if (g_hwndToolbar) {
            RECT toolbarRect;
            GetWindowRect(g_hwndToolbar, &toolbarRect);
            toolbarHeight = toolbarRect.bottom - toolbarRect.top;
        }
        
        // Create memory DC for double buffering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
        
        // Fill background with RGB(128, 0, 0)
        HBRUSH bgBrush = CreateSolidBrush(RGB(128, 0, 0));
        FillRect(memDC, &rect, bgBrush);
        DeleteObject(bgBrush);
        
        // Adjust rendering area for toolbar
        int renderHeight = rect.bottom - toolbarHeight;
        
        // Render scene
        RenderScene(memDC, rect.right, renderHeight);
        
        // Copy to screen
        BitBlt(hdc, 0, toolbarHeight, rect.right, renderHeight, memDC, 0, toolbarHeight, SRCCOPY);
        
        // Cleanup
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        
        EndPaint(hwnd, &ps);
        return 0;
    }
    
    case WM_TIMER:
        if (wParam == 1 && g_isAnimating) {
            UpdateRotation();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
        
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TOOLBAR_ANIMATE) {
            ToggleAnimation(hwnd);
        }
        return 0;
        
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_LEFT:
            g_rotationY -= 0.1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case VK_RIGHT:
            g_rotationY += 0.1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case VK_UP:
            g_rotationX -= 0.1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case VK_DOWN:
            g_rotationX += 0.1;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        return 0;
        
    case WM_ERASEBKGND:
        return 1;  // Prevent flicker
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Main entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Register window class
    const wchar_t CLASS_NAME[] = L"SphereGouraudWindow";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    // Create window
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"球体Gouraud光照模型",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
