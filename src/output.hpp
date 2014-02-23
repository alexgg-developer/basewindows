#ifndef _DX_OUTPUT_HPP_
#define _DX_OUTPUT_HPP_

#ifndef _WIN32_WINNT
  //#define _WIN32_WINNT   0x0501 // Windows XP
  //#define _WIN32_WINNT   0x0502 // Windows XP SP2
  //#define _WIN32_WINNT   0x0600 // Windows Vista
  #define _WIN32_WINNT   0x0601 // Windows 7
#endif

#define ReleaseCOM(x) { if(x){ x->Release();x = NULL; } }

#if defined(DEBUG) || defined(_DEBUG)
	#ifndef D3D_DEBUG_INFO
	#define D3D_DEBUG_INFO
	#endif
#endif

#include <d3dx10.h>
#include <dxerr.h>
#include <windows.h>
#include <string>
#include	"SceneManager.hpp"
#include	"ParticleManager.hpp"

//---------------------------------------------
// GLOBAL AND CONSTANT VARIABLES
//---------------------------------------------
const int   SCREEN_W                    = 1024;
const int   SCREEN_H                    = 768;
const bool  SETFULLSCREEN               = false;

extern float x,y,z;
extern float ax,ay,az;

//---------------------------------------------
// AUXILIAR STRUCTS
//---------------------------------------------


struct SimpleVertex
{
    D3DXVECTOR3 Pos;  // Position
    D3DXVECTOR4 Color;  
};

class output
{
  private:

    ID3D10Device*           md3dDevice;
    IDXGISwapChain*         mSwapChain;
    ID3D10Texture2D*        mDepthStencilBuffer;
    ID3D10RenderTargetView* mRenderTargetView;
    ID3D10DepthStencilView* mDepthStencilView;
    ID3D10InputLayout*      mVertexLayout;
    ID3D10Buffer*           mVertexBuffer;
    ID3D10Buffer*           mIndexBuffer;
    //Font
    ID3DX10Font*            mFont;
    //Backgrund color
    D3DXCOLOR               mClearColor;  
    //Constant matrices for shaders    
    ID3D10EffectMatrixVariable* mWorldVariable;
    ID3D10EffectMatrixVariable* mViewVariable  ;
    ID3D10EffectMatrixVariable* mProjectionVariable;
    D3DXMATRIX                  mWorld;
    D3DXMATRIX                  mView;
    D3DXMATRIX                  mProjection;
    //Shader stuff
    ID3D10Effect*           mEffect;
    ID3D10EffectTechnique*  mTechnique;

    HINSTANCE hInstance;
    HWND screen;
    int currentWidth,currentHeight;        
    HICON appIcon;
    bool keys[256];
    //Time
    double secondsPerCount;    
    float time;    
    std::wstring stringTime;

  private:

    bool initWindowsApp();
    bool initDirectx();
    void onResize();
    void drawScene();
    void getKeyboard();
    inline void updateText();
    inline void updateTime(const __int64& InitialCounts);

  public:

    //Creator
    output(HINSTANCE& handler);
    //Destructor
    ~output();
    //Operators
    bool init();
    int Main();
    //Windows
    LRESULT getMessage(UINT msg, WPARAM wParam, LPARAM lParam);
};
#endif //_DX_OUTPUT_HPP_