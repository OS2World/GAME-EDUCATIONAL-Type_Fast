/* typefast.h - TypeFast for OS/2 */

INT  main(VOID);
VOID AbortTypeFast(HWND hwndFrame, HWND hwndClient);
INT  notify(HWND hwndFrame, char *fmt, ...);

#define MSGBOXID    1001
#define ID_TIMER    1

#define ID_WINDOW   256
#define ID_RESOURCE ID_WINDOW

#define TIMER_EASY   1400
#define TIMER_MEDIUM  700
#define TIMER_HARD    300

/* Game menu */
#define IDM_SUB_GAME  110
#define IDM_NEWGAME   101
#define IDM_PAUSE     102
#define IDM_QUITGAME  103
#define IDM_EXIT      104

/* Options menu */
#define IDM_SUB_OPTIONS 210
#define IDM_SUB_DETAIL  211
#define IDM_SUB_LANG    212
#define IDM_EASY        201
#define IDM_MEDIUM      202
#define IDM_HARD        203
#define IDM_SOUND       204
#define IDM_CASE        205
#define IDM_BACKGRND    206
#define IDM_FRAME       207
#define IDM_SAVEONEXIT  208

/* Language menu */
#define IDM_LANG_EN  301
#define IDM_LANG_ES  302
#define IDM_LANG_NL  303
#define IDM_LANG_DE  304
#define IDM_LANG_FR  305
#define IDM_LANG_IT  306

/* Help menu */
#define IDM_SUB_HELP 910
#define IDM_ABOUT    999

/* About dialog */
#define IDD_ABOUT    2

#define N_LANG_STRINGS 18

/* Language IDs */
#define LANG_EN  0
#define LANG_ES  1
#define LANG_NL  2
#define LANG_DE  3
#define LANG_FR  4
#define LANG_IT  5

#ifndef RC_INVOKED
typedef struct {
    ULONG cbSize;
    INT   saveonexit;
    INT   curDiff;
    INT   iLang;
    INT   bSound;
    INT   bRespectCase;
    INT   bBackgrndRun;
} TFSETTINGS;
#endif
