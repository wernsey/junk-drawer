/*
Adapted from
http://bobobobo.wordpress.com/2009/03/30/adding-an-icon-system-tray-win32-c/
*/

/////////////////////////////////////////////
//                                         //
// Minimizing C++ Win32 App To System Tray //
//                                         //
// You found this at bobobobo's weblog,    //
// http://bobobobo.wordpress.com           //
//                                         //
// Creation date:  Mar 30/09               //
// Last modified:  Mar 30/09               //
//                                         //
/////////////////////////////////////////////

// GIVING CREDIT WHERE CREDIT IS DUE!!
// Thanks ubergeek!  http://www.gidforums.com/t-5815.html

#pragma region include and define
#include <windows.h>
#include <wtsapi32.h>
#include <shellapi.h>
#include <stdio.h>
#include <time.h>

#ifdef UNICODE
#define stringcopy wcscpy
#else
#define stringcopy strcpy
#endif

#include "bmp.h"
#include "screens.h"

#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  	3000
#define ID_TRAY_RESTORE_CONTEXT_MENU_ITEM	3001
#define ID_TRAY_ENABLE_CONTEXT_MENU_ITEM	3002
#define ID_TRAY_DISABLE_CONTEXT_MENU_ITEM	3003

#define WM_TRAYICON ( WM_USER + 1 )
#pragma endregion

#pragma region constants and globals
UINT WM_TASKBARCREATED = 0 ;

HWND g_hwnd ;
HMENU g_menu ;

NOTIFYICONDATA g_notifyIconData ;
#pragma endregion

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

#pragma region helper funcs

HICON icon1, icon2;

void Minimize() {
  Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
  ShowWindow(g_hwnd, SW_HIDE);
}

void Restore() {
  Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
  ShowWindow(g_hwnd, SW_SHOW);
}

void saveScreenshot();

int enabled = 1;

void InitNotifyIconData()
{
  memset( &g_notifyIconData, 0, sizeof( NOTIFYICONDATA ) ) ;
  
  g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
  
  g_notifyIconData.hWnd = g_hwnd;
  g_notifyIconData.uID = ID_TRAY_APP_ICON1;
  g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;      

  g_notifyIconData.uCallbackMessage = WM_TRAYICON;
  
  icon1 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_TRAY_APP_ICON1));
  icon2 = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_TRAY_APP_ICON2));
  
  g_notifyIconData.hIcon = enabled ? icon1 : icon2;
  stringcopy(g_notifyIconData.szTip, enabled ? TEXT("Screens is enabled") : TEXT("Screens is disabled"));
}
#pragma endregion

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR args, int iCmdShow )
{
  TCHAR className[] = TEXT( "tray icon class" );
  
  // listen for re-launching the taskbar. 
  WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated") ;
  
  #pragma region get window up
  WNDCLASSEX wnd = { 0 };

  wnd.hInstance = hInstance;
  wnd.lpszClassName = className;
  wnd.lpfnWndProc = WndProc;
  wnd.style = CS_HREDRAW | CS_VREDRAW ;
  wnd.cbSize = sizeof (WNDCLASSEX);

  wnd.hIcon = LoadIcon (NULL, IDI_APPLICATION);
  wnd.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
  wnd.hCursor = LoadCursor (NULL, IDC_ARROW);
  wnd.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE ;
  
  if (!RegisterClassEx(&wnd))
  {
    FatalAppExit( 0, TEXT("Couldn't register window class!") );
  }
  
  g_hwnd = CreateWindowEx (    
    0, className,    
    TEXT( APPLICATION_TITLE ),
    WS_OVERLAPPEDWINDOW,    
    CW_USEDEFAULT, CW_USEDEFAULT, 
    400, 400,
    NULL, NULL, 
    hInstance, NULL
  );

  InitNotifyIconData();

  WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);

  //RegisterHotKey(hwnd, 100, MOD_ALT | MOD_CONTROL, 'S');
  RegisterHotKey(g_hwnd, 100, MOD_ALT, 'S');
	
  Minimize();
  #pragma endregion

  MSG msg ;
  while (GetMessage (&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if( !IsWindowVisible( g_hwnd ) ) {
    Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
  }

  return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if ( message==WM_TASKBARCREATED && !IsWindowVisible( g_hwnd ) ) {
    Minimize();
    return 0;
  }

  switch (message)
  {
  case WM_CREATE:
  
    g_menu = CreatePopupMenu();

    AppendMenu(g_menu, MF_STRING, ID_TRAY_ENABLE_CONTEXT_MENU_ITEM,  TEXT( "Enable" ) );
    AppendMenu(g_menu, MF_STRING, ID_TRAY_DISABLE_CONTEXT_MENU_ITEM,  TEXT( "Disable" ) );
    AppendMenu(g_menu, MF_SEPARATOR, 0,  0 );
    AppendMenu(g_menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM,  TEXT( "Exit" ) );

    break;
  
  case WM_SYSCOMMAND:
    switch( wParam & 0xfff0 )  
    {
    case SC_MINIMIZE:
    case SC_CLOSE:
      Minimize() ; 
      return 0 ;
      break;
    }
    break;
  case WM_HOTKEY: {
    if(enabled) saveScreenshot();
	break;	
  }
  case WM_TRAYICON:
    {      
      switch(wParam) {
      case ID_TRAY_APP_ICON1:
        break;
      }

      if (lParam == WM_LBUTTONUP) {      
        POINT curPoint ;
        GetCursorPos( &curPoint );
        SetForegroundWindow(hwnd); 

        printf("calling track\n");
        UINT clicked = TrackPopupMenu(          
          g_menu,
          TPM_RETURNCMD | TPM_NONOTIFY, 
          curPoint.x,
          curPoint.y,
          0,
          hwnd,
          NULL
        );

        if (clicked == ID_TRAY_EXIT_CONTEXT_MENU_ITEM) {
          PostQuitMessage( 0 ) ;
        } else if (clicked == ID_TRAY_RESTORE_CONTEXT_MENU_ITEM) {
          Restore();
        } else if (clicked == ID_TRAY_ENABLE_CONTEXT_MENU_ITEM) {
		  enabled = 1;
		  Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
		  g_notifyIconData.hIcon = icon1;
		  stringcopy(g_notifyIconData.szTip, TEXT("Screens is enabled"));
		  Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
        } else if (clicked == ID_TRAY_DISABLE_CONTEXT_MENU_ITEM) {
		  enabled = 0;
		  Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
		  g_notifyIconData.hIcon = icon2;
		  stringcopy(g_notifyIconData.szTip, TEXT("Screens is disabled"));
		  Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);
        }		
		
      }
    }
    break;

  case WM_CLOSE:
    Minimize() ;
    return 0;
    break;

  case WM_DESTROY:
    PostQuitMessage (0);
    break;

  }

  return DefWindowProc( hwnd, message, wParam, lParam ) ;
}

void saveScreenshot() {
	// http://stackoverflow.com/a/3291411/115589
	HDC hScreenDC = CreateDC("DISPLAY", NULL, NULL, NULL);     
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
	int width = GetDeviceCaps(hScreenDC, HORZRES);
	int height = GetDeviceCaps(hScreenDC, VERTRES);
		
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof bmi);
	bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight =  -height; // Order pixels from top to bottom
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // last byte not used, 32 bit for alignment
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	bmi.bmiColors[0].rgbBlue = 0;
	bmi.bmiColors[0].rgbGreen = 0;
	bmi.bmiColors[0].rgbRed = 0;
	bmi.bmiColors[0].rgbReserved = 0;
	
	unsigned char *pixels;
	HBITMAP hbmp = CreateDIBSection( hMemoryDC, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0 );
	
	HBITMAP hOldBitmap = SelectObject(hMemoryDC, hbmp);

	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
	
	Bitmap *bmp = bm_bind(width, height, pixels);
	
	char filename[128];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	strftime(filename, sizeof filename, "screen-%Y%m%d%H%M%S.bmp", tmp);
		
	bm_save(bmp, filename);
	bm_unbind(bmp);
	
	DeleteObject(hbmp);

	DeleteDC(hMemoryDC);
	DeleteDC(hScreenDC);
}
