#include "output.hpp"
#include <sstream>

//Private vars

float x,y,z;
float ax,ay,az;

//Extern call
LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static output* app = 0;

	switch( msg )
	{
		case WM_CREATE:{			
			CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
			app = (output*)cs->lpCreateParams;
			return 0;
		}
	}

	if( app )
		return app->getMessage(msg, wParam, lParam);
	else
		return DefWindowProc(hwnd, msg, wParam, lParam);
}


//Creator
output::output(HINSTANCE& handler):
  screen(NULL),
  md3dDevice(NULL),
  mSwapChain(NULL),
  mDepthStencilBuffer(NULL),
  mRenderTargetView(NULL),
  mDepthStencilView(NULL),
  mVertexLayout(NULL),
  mVertexBuffer(NULL),
  mIndexBuffer(NULL),
  mClearColor(D3DXCOLOR(0.99f, 0.99f, 0.99f, 1.0f)),
  mEffect(NULL),
  mTechnique(NULL),
  mFont(NULL),
  appIcon(NULL),
  hInstance(handler),
  time(0.0f),
  secondsPerCount(0.0)
{
  stringTime = L"";  
  D3DXMatrixIdentity(&mView);
	D3DXMatrixIdentity(&mProjection);
	D3DXMatrixIdentity(&mWorld); 
  for (UINT point_i = 0; point_i < 256; ++point_i) 
    keys[point_i] = false;
  //Pending on erasing it
  y = z = x = -5.0f;
  ax = ay = 0.0f;
  az = 1.0f;
  //Set secondsPerCount
  __int64 countsPerSec;
  QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
  secondsPerCount = 1.0 / (double)countsPerSec;  
}

//Destructor

output::~output()
{
  ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);
	ReleaseCOM(md3dDevice);
  ReleaseCOM(mVertexLayout);
  ReleaseCOM(mVertexBuffer);  
  ReleaseCOM(mIndexBuffer);
  ReleaseCOM(mEffect);
  ReleaseCOM(mFont);

  DestroyIcon(appIcon);
}

int output::Main() //No se si los puedo pasar por referencia
{
  //Initialization
  if( !init() ) {
    //std::cerr << "Initialization failed" << std::endl;
    return 0;
  }
  MSG msg = {0};  

  __int64 initialCounts;
  QueryPerformanceCounter((LARGE_INTEGER*)&initialCounts);

  while(msg.message != WM_QUIT)
	{		
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}		
    else {
      updateTime(initialCounts);
      updateText();
      getKeyboard();
      drawScene();
    }
  }

  return (int)msg.wParam;
}

bool output::init()
{
  if( !SETFULLSCREEN ) {
    currentWidth  = 1024;
    currentHeight = 768;
  }
  if( !initWindowsApp() ) return false;
  if( !initDirectx() ) return false;

  return true;
}

bool output::initWindowsApp()
{
  //Icon
  appIcon =  ExtractIcon(hInstance,L"img\\icon.bmp",0);
  //Window class
  WNDCLASSEX wc;    
  wc.cbSize         = sizeof(WNDCLASSEX);
  wc.style          = CS_HREDRAW | CS_VREDRAW; 
  wc.lpfnWndProc    = WndProc;
  wc.cbClsExtra     = 0; 
  wc.cbWndExtra     = 0; 
  wc.hInstance      = hInstance;
  wc.hIcon          = appIcon;
  wc.hCursor        = LoadCursor(NULL,  IDC_ARROW); 
  wc.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
  wc.lpszMenuName   = 0;
  wc.lpszClassName  = L"IconWndClassEx"; 
  wc.hIconSm        = appIcon;

  //Registramos la instancia WNDCLASS en windows para que podamos crear ventanas basados en ella.
  if(!RegisterClassEx(&wc)){
    MessageBox(0, L"RegisterClass   FAILED", 0, 0);
    return false;
  }

  if( SETFULLSCREEN ) {
    ShowCursor(false);
  }

  RECT R = { 0, 0, currentWidth, currentHeight};
  AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

  screen = CreateWindow(
    L"IconWndClassEx",
    L"!Gordian", 
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, //Top left (0,0)
    CW_USEDEFAULT, 
    currentWidth, 
    currentHeight,
    0,
    0,
    hInstance,
    this); 

  if(screen == NULL){
   MessageBox(0, L"CreateWindow FAILED", 0, 0);
   return false;
  }
  
  ShowWindow(screen, SW_SHOW);
  UpdateWindow(screen);

  return true;
}

bool output::initDirectx()
{
  #if defined(DEBUG) | defined(_DEBUG)
	  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
  #endif
    
	DXGI_SWAP_CHAIN_DESC swapchain;
	swapchain.BufferDesc.Width                    = currentWidth;
	swapchain.BufferDesc.Height                   = currentHeight;
	swapchain.BufferDesc.RefreshRate.Numerator    = 60;
	swapchain.BufferDesc.RefreshRate.Denominator  = 1;
	swapchain.BufferDesc.Format                   = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchain.BufferDesc.ScanlineOrdering         = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapchain.BufferDesc.Scaling                  = DXGI_MODE_SCALING_UNSPECIFIED;
	// No multisampling.
	swapchain.SampleDesc.Count   = 1;
	swapchain.SampleDesc.Quality = 0;

	swapchain.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain.BufferCount  = 1;
	swapchain.OutputWindow = screen;
	swapchain.Windowed     = !SETFULLSCREEN;
	swapchain.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
	swapchain.Flags        = 0;

                    
  D3D10_DRIVER_TYPE md3dDriverType  = D3D10_DRIVER_TYPE_HARDWARE;	

	UINT createDeviceFlags = 0;
  #if defined(DEBUG) || defined(_DEBUG)  
      createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
  #endif

	D3D10CreateDeviceAndSwapChain(
			0,                 //default adapter
			md3dDriverType,
			0,                 // no software device
			createDeviceFlags, 
			D3D10_SDK_VERSION,
			&swapchain,
			&mSwapChain,
			&md3dDevice);  
  
  //FONT
  D3DX10_FONT_DESC fontDesc;

	fontDesc.Height          = 24;
  fontDesc.Width           = 0;
  fontDesc.Weight          = 0;
  fontDesc.MipLevels       = 1;
  fontDesc.Italic          = false;
  fontDesc.CharSet         = DEFAULT_CHARSET;
  fontDesc.OutputPrecision = OUT_DEFAULT_PRECIS;
  fontDesc.Quality         = DEFAULT_QUALITY;
  fontDesc.PitchAndFamily  = DEFAULT_PITCH | FF_DONTCARE;
  wcscpy_s(fontDesc.FaceName, L"Arial");  

	D3DX10CreateFontIndirect(md3dDevice, &fontDesc, &mFont);

  //Shaders
  DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;    
  #if defined( DEBUG ) || defined( _DEBUG )
    dwShaderFlags |= D3D10_SHADER_DEBUG;
  #endif
  HRESULT hr; //Used to check everything went ok (Windows only)
  hr = D3DX10CreateEffectFromFile( L"arturoresources\\dx\\shaders\\shader.fx", NULL, NULL, "fx_4_0", dwShaderFlags, 0,
                                         md3dDevice, NULL, NULL, &mEffect, NULL, NULL );
  if( FAILED( hr ) ) {
    MessageBox(0, L"Fx file hasn't been found", 0, 0);
    return false;
  }
  mTechnique          = mEffect->GetTechniqueByName( "Render" );

  mWorldVariable      = mEffect->GetVariableByName( "World" )->AsMatrix();
  mViewVariable       = mEffect->GetVariableByName( "View" )->AsMatrix();
  mProjectionVariable = mEffect->GetVariableByName( "Projection" )->AsMatrix();

  D3D10_INPUT_ELEMENT_DESC layout[] =  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 }
  };
  UINT numElements = sizeof( layout ) / sizeof( layout[0] );
  D3D10_PASS_DESC PassDesc;
  mTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );  
  hr = md3dDevice->CreateInputLayout( layout, numElements, PassDesc.pIAInputSignature,
                                        PassDesc.IAInputSignatureSize, &mVertexLayout );
  if( FAILED( hr ) ) return false;
  //Vertex buffer 
  SimpleVertex vertices[] =
  {
    { D3DXVECTOR3( -1.0f, 1.0f, -1.0f ), D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 1.0f ) },
    { D3DXVECTOR3( 1.0f, 1.0f, -1.0f ), D3DXVECTOR4( 0.0f, 1.0f, 0.0f, 1.0f ) },
    { D3DXVECTOR3( 1.0f, 1.0f, 1.0f ), D3DXVECTOR4( 0.0f, 1.0f, 1.0f, 1.0f ) },
    { D3DXVECTOR3( -1.0f, 1.0f, 1.0f ), D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 1.0f ) },
    { D3DXVECTOR3( -1.0f, -1.0f, -1.0f ), D3DXVECTOR4( 1.0f, 0.0f, 1.0f, 1.0f ) },
    { D3DXVECTOR3( 1.0f, -1.0f, -1.0f ), D3DXVECTOR4( 1.0f, 1.0f, 0.0f, 1.0f ) },
    { D3DXVECTOR3( 1.0f, -1.0f, 1.0f ), D3DXVECTOR4( 1.0f, 1.0f, 1.0f, 1.0f ) },
    { D3DXVECTOR3( -1.0f, -1.0f, 1.0f ), D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 1.0f ) },
  };
  /*vec<Particle,8> p;
  p[0].Pos = RawTovec( -1.0f, 1.0f, -1.0f );  p[0].Color = RawTovec( 0.0f, 0.0f, 1.0f, 1.0f );
  p[1].Pos = RawTovec( 1.0f, 1.0f, -1.0f );   p[1].Color = RawTovec( 0.0f, 1.0f, 0.0f, 1.0f );
  p[2].Pos = RawTovec( 1.0f, 1.0f, 1.0f );    p[2].Color = RawTovec( 0.0f, 1.0f, 1.0f, 1.0f );
  p[3].Pos = RawTovec( -1.0f, 1.0f, 1.0f );   p[3].Color = RawTovec( 1.0f, 0.0f, 0.0f, 1.0f );
  p[4].Pos = RawTovec( -1.0f, -1.0f, -1.0f ); p[4].Color = RawTovec( 1.0f, 0.0f, 1.0f, 1.0f );
  p[5].Pos = RawTovec( 1.0f, -1.0f, -1.0f );  p[5].Color = RawTovec( 1.0f, 1.0f, 0.0f, 1.0f );
  p[6].Pos = RawTovec( 1.0f, -1.0f, 1.0f );   p[6].Color = RawTovec( 1.0f, 1.0f, 1.0f, 1.0f );
  p[7].Pos = RawTovec( -1.0f, -1.0f, 1.0f );  p[7].Color = RawTovec( 0.0f, 0.0f, 0.0f, 1.0f );*/

  D3D10_BUFFER_DESC bd;
  D3D10_SUBRESOURCE_DATA InitData;

  bd.Usage = D3D10_USAGE_DEFAULT;
  bd.ByteWidth = sizeof( SimpleVertex ) * 8;
  bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
  bd.CPUAccessFlags = 0;
  bd.MiscFlags = 0;

  InitData.pSysMem = vertices;
  hr = md3dDevice->CreateBuffer( &bd, &InitData, &mVertexBuffer );
  if( FAILED( hr ) ) return false;
  //Index buffer
  DWORD indices[] =
  {
      3,1,0,
      2,1,3,

      0,5,4,
      1,5,0,

      3,4,7,
      0,4,3,

      1,6,5,
      2,6,1,

      2,7,6,
      3,7,2,

      6,4,5,
      7,4,6,
  };

  bd.Usage = D3D10_USAGE_DEFAULT;
  bd.ByteWidth = sizeof( DWORD ) * 36;
  bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
  bd.CPUAccessFlags = 0;
  bd.MiscFlags = 0;
  
  InitData.pSysMem = indices;
  hr = md3dDevice->CreateBuffer( &bd, &InitData, &mIndexBuffer );
  if( FAILED( hr ) ) return false;

	onResize();

  return true;
}

void output::onResize()
{
	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mDepthStencilBuffer);
	// Resize the swap chain and recreate the render target view.
	mSwapChain->ResizeBuffers(1, currentWidth, currentHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	ID3D10Texture2D* backBuffer;
	mSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), reinterpret_cast<void**>(&backBuffer));
	md3dDevice->CreateRenderTargetView(backBuffer, 0, &mRenderTargetView);
	ReleaseCOM(backBuffer);
	// Create the depth/stencil buffer and view.
	D3D10_TEXTURE2D_DESC depthStencilDesc;
	
	depthStencilDesc.Width              = currentWidth;
	depthStencilDesc.Height             = currentHeight;
	depthStencilDesc.MipLevels          = 1;
	depthStencilDesc.ArraySize          = 1;
	depthStencilDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count   = 1; // multisampling must match
	depthStencilDesc.SampleDesc.Quality = 0; // swap chain values.
	depthStencilDesc.Usage              = D3D10_USAGE_DEFAULT;
	depthStencilDesc.BindFlags          = D3D10_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags     = 0; 
	depthStencilDesc.MiscFlags          = 0;

	md3dDevice->CreateTexture2D(&depthStencilDesc, 0, &mDepthStencilBuffer);
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer, 0, &mDepthStencilView);
	// Bind the render target view and depth/stencil view to the pipeline.
	md3dDevice->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	// Set the viewport transform.
	D3D10_VIEWPORT vp;

	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width    = currentWidth;
	vp.Height   = currentHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	md3dDevice->RSSetViewports(1, &vp);  

  D3DXMatrixPerspectiveFovLH( &mProjection, ( float )D3DX_PI * 0.5f, currentWidth / (FLOAT)currentHeight, 0.1f, 100.0f );
 }

void output::drawScene()
{  
  md3dDevice->ClearRenderTargetView(mRenderTargetView, mClearColor);
	md3dDevice->ClearDepthStencilView(mDepthStencilView, D3D10_CLEAR_DEPTH|D3D10_CLEAR_STENCIL, 1.0f, 0);
  md3dDevice->OMSetDepthStencilState(0, 0);

  md3dDevice->IASetInputLayout(mVertexLayout);
  md3dDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  
  //Render stuff!
  mWorldVariable->SetMatrix( (float*)&mWorld );
  mViewVariable->SetMatrix( (float*)&mView );
  mProjectionVariable->SetMatrix( (float*)&mProjection );

  D3D10_TECHNIQUE_DESC techDesc;
  mTechnique->GetDesc( &techDesc );
  for( UINT p = 0; p < techDesc.Passes; ++p )  {
    mTechnique->GetPassByIndex( p )->Apply( 0 );
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    md3dDevice->IASetVertexBuffers( 0, 1, &mVertexBuffer, &stride, &offset );  
    md3dDevice->IASetIndexBuffer( mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);    
    md3dDevice->DrawIndexed( 36, 0, 0 );
  }

  //Draw text  
	RECT R = {5, 5, 0, 0};
  D3DXCOLOR BLACK(0.0f, 0.0f, 0.0f, 1.0f);
  mFont->DrawText(0, stringTime.c_str(), -1, &R, DT_NOCLIP, BLACK);

  //Draw the backbuffer
  mSwapChain->Present( 0, 0 ); 
}

void output::getKeyboard()
{
  if(keys[VK_UP]) {
    ay = ay + 0.001f;
  }
  if(keys[VK_RIGHT]) {
    ax = ax + 0.001f;
  }  
  if(keys[VK_DOWN]) {
    ay = ay - 0.001f;
  }
  if(keys[VK_LEFT]) {
    ax = ax - 0.001f;
  }
  
  if(keys['W']) {
    z = z + 0.001f;
  }
  if(keys['S']) {
    z = z - 0.001f;
  }
  if(keys['A']) {
    ax = ax - 0.001f;
  }
  if(keys['D']) {
    ax = ax + 0.001f;
  }
  //Camera
  D3DXVECTOR3 Eye( x, y, z);
  //Where is looking at
  D3DXVECTOR3 At( x+ax, y+ay, z);
  //Where is +Y
  D3DXVECTOR3 Up( 0.0f, 1.0f, 0.0f );
  D3DXMatrixLookAtLH( &mView, &Eye, &At, &Up );  

}

void output::updateTime(const __int64& initialCounts)
{
  __int64 currCounts;
  QueryPerformanceCounter((LARGE_INTEGER*)&currCounts);
  time = (float)((currCounts-initialCounts)*secondsPerCount); 
}

void output::updateText()
{
  std::wostringstream outs;   
  outs.precision(6);
  outs << L"Time elapsed in seconds: " << time << L"\n";
  outs << L"Width: " << currentWidth << L" Height: " << currentHeight; 
  stringTime = outs.str();
}
//Extern, it's called from the SO
LRESULT output::getMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{  
  switch(msg){
    case WM_LBUTTONDOWN:      
      return 0;
    case WM_KEYDOWN:
      if( wParam == VK_ESCAPE ) DestroyWindow(screen);
      else keys[wParam] = true;
      return 0;
    case WM_KEYUP:
      keys[wParam] = false; 
      return 0;    
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  	case WM_SYSCOMMAND:
			switch (wParam)
			{
				case SC_SCREENSAVE:	
				case SC_MONITORPOWER:	
				return 0;	
			}
			break;	
    case WM_SIZE:       
	    currentWidth  = LOWORD(lParam);
	    currentHeight = HIWORD(lParam);
      if( md3dDevice ) {			  
			  if( wParam == SIZE_RESTORED ){  				
					  onResize();
				}
      }
      return 0;
  }  
  return DefWindowProc(screen, msg, wParam, lParam);
}
