#define ID_APP_ICON 5000
#define ID_TRAY_APP_ICON1 5001
#define ID_TRAY_APP_ICON2 5002

#define APPLICATION_TITLE "Screenshot"

/*
http://blogs.msdn.com/b/calvin_hsia/archive/2005/08/02/446742.aspx
https://github.com/msysgit/msysgit/blob/master/mingw/include/wtsapi32.h
*/
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8
#define WTS_SESSION_REMOTE_CONTROL         0x9
#define NOTIFY_FOR_ALL_SESSIONS     1
#define NOTIFY_FOR_THIS_SESSION     0
 
#define WM_WTSSESSION_CHANGE            0x02B1

BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags);