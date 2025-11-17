// SphereGouraudDlg.cpp : implementation file
//
#pragma execution_character_set("utf-8")

#include "pch.h"
#include "framework.h"
#include "SphereGouraud.h"
#include "SphereGouraudDlg.h"
#include "afxdialogex.h"
#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Constants
const double PI = 3.14159265358979323846;
const int SPHERE_SEGMENTS_LAT = 30;
const int SPHERE_SEGMENTS_LON = 40;
const double SPHERE_RADIUS = 200.0;

// CSphereGouraudDlg dialog

CSphereGouraudDlg::CSphereGouraudDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SPHEREGOURAUD_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    
    // Initialize variables
    m_zBufferWidth = 0;
    m_zBufferHeight = 0;
    m_rotationX = 0.0;
    m_rotationY = 0.0;
    m_rotationZ = 0.0;
    m_isAnimating = false;
    m_timerID = 0;
    
    // Light source (front upper right - closer to viewer for better lighting)
    m_lightPos = Vector3(300.0, 400.0, 200.0);  // Reduced Z from 500 to 200
    
    // Material properties - Bright colors for clear visibility
    m_ambientColor = Vector3(0.3, 0.3, 0.3);  // Increased ambient
    m_diffuseColor = Vector3(0.9, 0.9, 0.9);  // Very bright gray for visibility
    m_specularColor = Vector3(1.0, 1.0, 1.0);
    m_shininess = 32.0;
    
    // Light properties - Brighter lighting
    m_lightAmbient = Vector3(0.5, 0.5, 0.5);  // Increased ambient light
    m_lightDiffuse = Vector3(1.0, 1.0, 1.0);
    m_lightSpecular = Vector3(1.0, 1.0, 1.0);
}

void CSphereGouraudDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSphereGouraudDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_ANIMATE, &CSphereGouraudDlg::OnBnClickedAnimate)
    ON_WM_KEYDOWN()
    ON_WM_DESTROY()
END_MESSAGE_MAP()


// CSphereGouraudDlg message handlers

BOOL CSphereGouraudDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Initialize sphere mesh
    CreateSphereMesh();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

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

// Create sphere mesh
void CSphereGouraudDlg::CreateSphereMesh() {
    m_vertices.clear();
    m_triangles.clear();
    
    // Create vertices
    for (int lat = 0; lat <= SPHERE_SEGMENTS_LAT; lat++) {
        double theta = PI * lat / SPHERE_SEGMENTS_LAT;
        double sinTheta = sin(theta);
        double cosTheta = cos(theta);
        
        for (int lon = 0; lon <= SPHERE_SEGMENTS_LON; lon++) {
            double phi = 2.0 * PI * lon / SPHERE_SEGMENTS_LON;
            double sinPhi = sin(phi);
            double cosPhi = cos(phi);
            
            Vertex vertex;
            vertex.position.x = SPHERE_RADIUS * sinTheta * cosPhi;
            vertex.position.y = SPHERE_RADIUS * cosTheta;
            vertex.position.z = SPHERE_RADIUS * sinTheta * sinPhi;
            
            vertex.normal = vertex.position.normalize();
            
            m_vertices.push_back(vertex);
        }
    }
    
    // Create triangles
    for (int lat = 0; lat < SPHERE_SEGMENTS_LAT; lat++) {
        for (int lon = 0; lon < SPHERE_SEGMENTS_LON; lon++) {
            int first = lat * (SPHERE_SEGMENTS_LON + 1) + lon;
            int second = first + SPHERE_SEGMENTS_LON + 1;
            
            Triangle tri1;
            tri1.v0 = first;
            tri1.v1 = second;
            tri1.v2 = first + 1;
            m_triangles.push_back(tri1);
            
            Triangle tri2;
            tri2.v0 = second;
            tri2.v1 = second + 1;
            tri2.v2 = first + 1;
            m_triangles.push_back(tri2);
        }
    }
}

// Calculate vertex color (Gouraud shading)
COLORREF CSphereGouraudDlg::CalculateVertexColor(const Vertex& vertex) {
    Vector3 lightDir = (m_lightPos - vertex.transformedPos).normalize();
    Vector3 viewDir = Vector3(0, 0, 1);
    
    // Ambient
    Vector3 ambient = Vector3(
        m_ambientColor.x * m_lightAmbient.x,
        m_ambientColor.y * m_lightAmbient.y,
        m_ambientColor.z * m_lightAmbient.z
    );
    
    // Diffuse
    double diff = std::max(0.0, vertex.transformedNormal.dot(lightDir));
    Vector3 diffuse = Vector3(
        m_diffuseColor.x * m_lightDiffuse.x * diff,
        m_diffuseColor.y * m_lightDiffuse.y * diff,
        m_diffuseColor.z * m_lightDiffuse.z * diff
    );
    
    // Specular (Blinn-Phong)
    Vector3 halfDir = (lightDir + viewDir).normalize();
    double spec = pow(std::max(0.0, vertex.transformedNormal.dot(halfDir)), m_shininess);
    Vector3 specular = Vector3(
        m_specularColor.x * m_lightSpecular.x * spec,
        m_specularColor.y * m_lightSpecular.y * spec,
        m_specularColor.z * m_lightSpecular.z * spec
    );
    
    // Combine
    Vector3 color = ambient + diffuse + specular;
    
    int r = std::min(255, std::max(0, (int)(color.x * 255)));
    int g = std::min(255, std::max(0, (int)(color.y * 255)));
    int b = std::min(255, std::max(0, (int)(color.z * 255)));
    
    return RGB(r, g, b);
}

// Transform vertices
void CSphereGouraudDlg::TransformVertices(int width, int height) {
    int centerX = width / 2;
    int centerY = height / 2;
    
    for (size_t i = 0; i < m_vertices.size(); i++) {
        Vertex& v = m_vertices[i];
        
        v.transformedPos = v.position;
        v.transformedNormal = v.normal;
        
        RotateX(v.transformedPos, m_rotationX);
        RotateY(v.transformedPos, m_rotationY);
        RotateZ(v.transformedPos, m_rotationZ);
        
        RotateX(v.transformedNormal, m_rotationX);
        RotateY(v.transformedNormal, m_rotationY);
        RotateZ(v.transformedNormal, m_rotationZ);
        v.transformedNormal = v.transformedNormal.normalize();
        
        v.color = CalculateVertexColor(v);
        
        v.screenX = centerX + (int)v.transformedPos.x;
        v.screenY = centerY - (int)v.transformedPos.y;
        v.screenZ = v.transformedPos.z;
    }
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

// Draw triangle with Gouraud shading
void CSphereGouraudDlg::DrawTriangleGouraud(CDC* pDC, const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    // Backface culling
    Vector3 edge1 = v1.transformedPos - v0.transformedPos;
    Vector3 edge2 = v2.transformedPos - v0.transformedPos;
    Vector3 normal = edge1.cross(edge2);
    
    Vector3 viewDir(0, 0, 1);
    if (normal.dot(viewDir) <= 0) return;
    
    // Bounding box
    int minX = std::min(v0.screenX, std::min(v1.screenX, v2.screenX));
    int maxX = std::max(v0.screenX, std::max(v1.screenX, v2.screenX));
    int minY = std::min(v0.screenY, std::min(v1.screenY, v2.screenY));
    int maxY = std::max(v0.screenY, std::max(v1.screenY, v2.screenY));
    
    minX = std::max(0, minX);
    maxX = std::min(m_zBufferWidth - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(m_zBufferHeight - 1, maxY);
    
    // Rasterize
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            double w0, w1, w2;
            GetBarycentricCoords(x, y, v0.screenX, v0.screenY, v1.screenX, v1.screenY,
                               v2.screenX, v2.screenY, w0, w1, w2);
            
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                double z = v0.screenZ * w0 + v1.screenZ * w1 + v2.screenZ * w2;
                
                int bufferIndex = y * m_zBufferWidth + x;
                // Add small epsilon to reduce Z-fighting artifacts at edges
                if (bufferIndex >= 0 && bufferIndex < (int)m_zBuffer.size() && z > m_zBuffer[bufferIndex] + 0.001) {
                    m_zBuffer[bufferIndex] = z;
                    
                    COLORREF color = InterpolateColor(v0.color, v1.color, v2.color, w0, w1, w2);
                    pDC->SetPixel(x, y, color);
                }
            }
        }
    }
}

// Render scene
void CSphereGouraudDlg::RenderScene(CDC* pDC, int width, int height) {
    if (m_zBufferWidth != width || m_zBufferHeight != height) {
        m_zBufferWidth = width;
        m_zBufferHeight = height;
        m_zBuffer.resize(width * height);
    }
    std::fill(m_zBuffer.begin(), m_zBuffer.end(), -10000.0);
    
    TransformVertices(width, height);
    
    for (const Triangle& tri : m_triangles) {
        const Vertex& v0 = m_vertices[tri.v0];
        const Vertex& v1 = m_vertices[tri.v1];
        const Vertex& v2 = m_vertices[tri.v2];
        
        DrawTriangleGouraud(pDC, v0, v1, v2);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSphereGouraudDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
        CPaintDC dc(this);
        CRect rect;
        GetClientRect(&rect);
        
        // Adjust for button area
        int buttonHeight = 40;
        int renderHeight = rect.Height() - buttonHeight;
        
        // Create memory DC
        CDC memDC;
        memDC.CreateCompatibleDC(&dc);
        CBitmap bitmap;
        bitmap.CreateCompatibleBitmap(&dc, rect.Width(), renderHeight);
        CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);
        
        // Fill background with RGB(128, 0, 0)
        memDC.FillSolidRect(0, 0, rect.Width(), renderHeight, RGB(128, 0, 0));
        
        // Render scene
        RenderScene(&memDC, rect.Width(), renderHeight);
        
        // Copy to screen
        dc.BitBlt(0, buttonHeight, rect.Width(), renderHeight, &memDC, 0, 0, SRCCOPY);
        
        // Cleanup
        memDC.SelectObject(pOldBitmap);
        
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSphereGouraudDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSphereGouraudDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1 && m_isAnimating) {
        UpdateRotation();
        Invalidate(FALSE);
    }
    
    CDialogEx::OnTimer(nIDEvent);
}

void CSphereGouraudDlg::UpdateRotation()
{
    m_rotationY += 0.02;
    if (m_rotationY > 2.0 * PI) m_rotationY -= 2.0 * PI;
}

void CSphereGouraudDlg::OnBnClickedAnimate()
{
    m_isAnimating = !m_isAnimating;
    
    if (m_isAnimating) {
        m_timerID = SetTimer(1, 16, NULL);
        GetDlgItem(IDC_ANIMATE)->SetWindowText(_T("停止"));
    } else {
        if (m_timerID != 0) {
            KillTimer(m_timerID);
            m_timerID = 0;
        }
        GetDlgItem(IDC_ANIMATE)->SetWindowText(_T("动画"));
    }
}

void CSphereGouraudDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar) {
    case VK_LEFT:
        m_rotationY -= 0.1;
        Invalidate(FALSE);
        break;
    case VK_RIGHT:
        m_rotationY += 0.1;
        Invalidate(FALSE);
        break;
    case VK_UP:
        m_rotationX -= 0.1;
        Invalidate(FALSE);
        break;
    case VK_DOWN:
        m_rotationX += 0.1;
        Invalidate(FALSE);
        break;
    }
    
    CDialogEx::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CSphereGouraudDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        OnKeyDown((UINT)pMsg->wParam, (UINT)LOWORD(pMsg->lParam), (UINT)HIWORD(pMsg->lParam));
        return TRUE;
    }
    
    return CDialogEx::PreTranslateMessage(pMsg);
}

void CSphereGouraudDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
    
    if (m_timerID != 0) {
        KillTimer(m_timerID);
        m_timerID = 0;
    }
}
