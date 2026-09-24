/* TypeFast for OS/2 PM - originally by Turgut Kalfaoglu, 1992     */
/* OpenWatcom 32-bit PM build, OS2World 2026                        */

#define INCL_WIN
#define INCL_GPI
#define INCL_DOSDATETIME
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#include <os2.h>
#include "typefast.h"
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

const char bldlevel[] =
    "@#OS2World:1.2#@##1## " __DATE__
    " TypeFast - Typing practice game for OS/2::::@@TypeFast";

MRESULT EXPENTRY MyWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
CHAR   *findword(void);
INT     readline(char *p);
BOOL    goodword(char *p);
VOID    load_settings(void);
VOID    save_settings(void);
VOID    apply_diff_check(HWND hwndMenu, INT prev, INT next);
VOID    apply_checked(HWND hwndMenu, INT id, BOOL on);
VOID    frame_show(HWND hwndClient, BOOL show);
VOID    set_menu_lang(HWND hwndMenu, INT lang);

HAB  hab;
PSZ  pszErrMsg;

/* Game state */
POINTL pt;
CHAR  *words[30], kbbuf[256];
INT    wordx[30], wordy[30];
INT    activewords, lost, score, maxactivewords;
INT    timertime, curDiff;
LONG   CharHeight, CharWidth, topx, topy;
FILE  *wordfile;
BOOL   anykeyhit, pausemode, gameover, lastwordheight;
ULONG  wordcount, wordcountcorr;
float  startsec;

/* Persistent settings */
BOOL sound        = TRUE;
BOOL respectCase  = FALSE;
BOOL bBackgrndRun = FALSE;
BOOL bSaveOnexit  = FALSE;
INT  iLang        = LANG_EN;

VOID load_settings(void)
{
    TFSETTINGS cfg;
    FILE *f = fopen("TYPEFAST.CFG", "rb");
    if (!f) return;
    if (fread(&cfg, sizeof cfg, 1, f) == 1 && cfg.cbSize == sizeof cfg) {
        bSaveOnexit  = (BOOL)cfg.saveonexit;
        iLang        = cfg.iLang;
        sound        = (BOOL)cfg.bSound;
        respectCase  = (BOOL)cfg.bRespectCase;
        bBackgrndRun = (BOOL)cfg.bBackgrndRun;
        switch (cfg.curDiff) {
        case IDM_MEDIUM: curDiff = IDM_MEDIUM; timertime = TIMER_MEDIUM; break;
        case IDM_HARD:   curDiff = IDM_HARD;   timertime = TIMER_HARD;   break;
        default:         curDiff = IDM_EASY;   timertime = TIMER_EASY;   break;
        }
    }
    fclose(f);
}

VOID save_settings(void)
{
    TFSETTINGS cfg;
    FILE *f = fopen("TYPEFAST.CFG", "wb");
    if (!f) return;
    cfg.cbSize      = sizeof cfg;
    cfg.saveonexit  = bSaveOnexit;
    cfg.curDiff     = curDiff;
    cfg.iLang       = iLang;
    cfg.bSound      = sound;
    cfg.bRespectCase= respectCase;
    cfg.bBackgrndRun= bBackgrndRun;
    fwrite(&cfg, sizeof cfg, 1, f);
    fclose(f);
}

VOID apply_diff_check(HWND hwndMenu, INT prev, INT next)
{
    WinSendMsg(hwndMenu, MM_SETITEMATTR,
               MPFROM2SHORT(prev, TRUE), MPFROM2SHORT(MIA_CHECKED, 0));
    WinSendMsg(hwndMenu, MM_SETITEMATTR,
               MPFROM2SHORT(next, TRUE), MPFROM2SHORT(MIA_CHECKED, MIA_CHECKED));
}

VOID apply_checked(HWND hwndMenu, INT id, BOOL on)
{
    WinSendMsg(hwndMenu, MM_SETITEMATTR,
               MPFROM2SHORT(id, TRUE),
               MPFROM2SHORT(MIA_CHECKED, on ? MIA_CHECKED : 0));
}

static const char * const lang_strings[6][N_LANG_STRINGS] = {
/* EN */ {
 "~Game","~New Game\tCtrl+N","~Pause\tCtrl+P","~Quit Game\tCtrl+Q","E~xit\tCtrl+X",
 "~Options","~Difficulty","~Easy","~Medium","~Hard",
 "~Sound\tCtrl+S","Respect ~Case\tCtrl+C","~Language",
 "~Background Run\tCtrl+B","~Frame Controls\tCtrl+F","~Save settings on exit",
 "~Help","~About TypeFast..."
},
/* ES */ {
 "~Juego","~Nuevo Juego\tCtrl+N","~Pausa\tCtrl+P","~Terminar Juego\tCtrl+Q","~Salir\tCtrl+X",
 "~Opciones","~Dificultad","~Facil","~Medio","~Dificil",
 "~Sonido\tCtrl+S","~Mayusculas\tCtrl+C","~Idioma",
 "~Segundo Plano\tCtrl+B","~Marco\tCtrl+F","~Guardar opciones al salir",
 "~Ayuda","~Acerca de TypeFast..."
},
/* NL */ {
 "~Spel","~Nieuw Spel\tCtrl+N","~Pauze\tCtrl+P","~Spel Stoppen\tCtrl+Q","~Afsluiten\tCtrl+X",
 "~Opties","~Moeilijkheid","~Makkelijk","~Gemiddeld","~Moeilijk",
 "~Geluid\tCtrl+S","~Hoofdletters\tCtrl+C","~Taal",
 "~Achtergrond\tCtrl+B","~Vensterrand\tCtrl+F","~Instellingen bewaren",
 "~Help","~Over TypeFast..."
},
/* DE */ {
 "~Spiel","~Neues Spiel\tCtrl+N","~Pause\tCtrl+P","~Spiel Beenden\tCtrl+Q","~Beenden\tCtrl+X",
 "~Optionen","~Schwierigkeit","~Einfach","~Mittel","~Schwer",
 "~Sound\tCtrl+S","~Grossschreibung\tCtrl+C","~Sprache",
 "~Hintergrund\tCtrl+B","~Rahmen\tCtrl+F","~Einstellungen speichern",
 "~Hilfe","~Ueber TypeFast..."
},
/* FR */ {
 "~Jeu","~Nouveau Jeu\tCtrl+N","~Pause\tCtrl+P","~Quitter le Jeu\tCtrl+Q","~Fermer\tCtrl+X",
 "~Options","~Difficulte","~Facile","~Moyen","~Difficile",
 "~Son\tCtrl+S","~Majuscules\tCtrl+C","~Langue",
 "~Fond\tCtrl+B","~Cadre\tCtrl+F","~Sauvegarder les options",
 "~Aide","~A propos de TypeFast..."
},
/* IT */ {
 "~Gioco","~Nuovo Gioco\tCtrl+N","~Pausa\tCtrl+P","~Abbandona\tCtrl+Q","~Esci\tCtrl+X",
 "~Opzioni","~Difficolta","~Facile","~Medio","~Difficile",
 "~Suono\tCtrl+S","~Maiuscole\tCtrl+C","~Lingua",
 "~Sfondo\tCtrl+B","~Cornice\tCtrl+F","~Salva opzioni all'uscita",
 "~Aiuto","~Informazioni su TypeFast..."
}
};

static const INT lang_ids[N_LANG_STRINGS] = {
    IDM_SUB_GAME,    IDM_NEWGAME,    IDM_PAUSE,      IDM_QUITGAME,   IDM_EXIT,
    IDM_SUB_OPTIONS, IDM_SUB_DETAIL, IDM_EASY,       IDM_MEDIUM,     IDM_HARD,
    IDM_SOUND,       IDM_CASE,       IDM_SUB_LANG,
    IDM_BACKGRND,    IDM_FRAME,      IDM_SAVEONEXIT,
    IDM_SUB_HELP,    IDM_ABOUT
};

VOID set_menu_lang(HWND hwndMenu, INT lang)
{
    INT i;
    if (lang < 0 || lang >= 6) lang = LANG_EN;
    for (i = 0; i < N_LANG_STRINGS; i++)
        WinSendMsg(hwndMenu, MM_SETITEMTEXT,
                   MPFROM2SHORT(lang_ids[i], TRUE),
                   MPFROMP((PSZ)lang_strings[lang][i]));
}

VOID frame_show(HWND hwndClient, BOOL show)
{
    HWND   hwndFrm = WinQueryWindow(hwndClient, QW_PARENT);
    USHORT usFlags = WinQueryWindowUShort(hwndFrm, QWS_FLAGS);
    ULONG  ulCtl   = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX | FCF_MENU;

    if (show)
        usFlags |= (USHORT)ulCtl;
    else
        usFlags &= (USHORT)~ulCtl;

    WinSetWindowUShort(hwndFrm, QWS_FLAGS, usFlags);
    WinSendMsg(hwndFrm, WM_UPDATEFRAME, MPFROMLONG(ulCtl), 0);
}

INT init(void)
{
    INT i;
    activewords = 0;
    lost = 0; score = 0; maxactivewords = 4;
    topx = 640; topy = 480;
    anykeyhit = 0; pausemode = 0; gameover = 0;
    wordcount = 0; wordcountcorr = 0; lastwordheight = 0;
    kbbuf[0] = 0;
    for (i = 0; i < 30; i++) wordx[i] = 0;
    /* reset speed based on current difficulty */
    switch (curDiff) {
    case IDM_MEDIUM: timertime = TIMER_MEDIUM; break;
    case IDM_HARD:   timertime = TIMER_HARD;   break;
    default:         timertime = TIMER_EASY;   break;
    }
    return 0;
}

INT main(VOID)
{
    HMQ  hmq  = NULLHANDLE;
    HWND hwndClient = NULLHANDLE;
    HWND hwndFrame  = NULLHANDLE;
    QMSG qmsg;
    ULONG flCreate;
    RECTL rcl;
    LONG  scx, scy, cx, cy, x, y;

    curDiff   = IDM_EASY;
    timertime = TIMER_EASY;

    init();
    load_settings();

    if ((hab = WinInitialize(0)) == 0L)
        AbortTypeFast(hwndFrame, hwndClient);

    if ((hmq = WinCreateMsgQueue(hab, 0)) == 0L)
        AbortTypeFast(hwndFrame, hwndClient);

    if (!WinRegisterClass(hab, (PSZ)"TypeFastWnd",
                          (PFNWP)MyWindowProc, CS_SIZEREDRAW, 0))
        AbortTypeFast(hwndFrame, hwndClient);

    flCreate = FCF_STANDARD & ~FCF_SHELLPOSITION;

    if ((hwndFrame = WinCreateStdWindow(
                HWND_DESKTOP, 0L, &flCreate,
                "TypeFastWnd", "TypeFast for OS/2",
                0L, (HMODULE)0L, ID_RESOURCE,
                &hwndClient)) == 0L)
        AbortTypeFast(hwndFrame, hwndClient);

    /* Size 1024x768 and center */
    rcl.xLeft = 0; rcl.yBottom = 0; rcl.xRight = 1024; rcl.yTop = 768;
    WinCalcFrameRect(hwndFrame, &rcl, FALSE);
    cx = rcl.xRight - rcl.xLeft;
    cy = rcl.yTop   - rcl.yBottom;
    scx = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
    scy = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
    if (cx > scx) cx = scx;
    if (cy > scy) cy = scy;
    x = (scx - cx) / 2;
    y = (scy - cy) / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    WinSetWindowPos(hwndFrame, HWND_TOP, x, y, cx, cy,
                    SWP_MOVE | SWP_SIZE | SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER);

    WinStartTimer(hab, hwndClient, ID_TIMER, timertime);

    while (WinGetMsg(hab, &qmsg, 0L, 0, 0))
        WinDispatchMsg(hab, &qmsg);

    WinStopTimer(hab, hwndClient, ID_TIMER);
    WinDestroyWindow(hwndFrame);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
    return 0;
}

MRESULT EXPENTRY MyWindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    DATETIME dt;
    static HWND hwndMenu = NULLHANDLE;
    static BOOL bFrameVisible = TRUE;

    switch (msg)
    {
    case WM_CREATE:
    {
        INT i;
        FONTMETRICS fm;
        HPS hps;

        hps = WinGetPS(hwnd);
        GpiQueryFontMetrics(hps, (LONG)sizeof fm, &fm);
        CharHeight = (INT)fm.lMaxBaselineExt;
        CharWidth  = (INT)fm.lMaxCharInc;
        if (CharHeight < 1 || CharHeight > 50)
            notify(hwnd, "CharHeight is %d!", CharHeight);
        if (CharWidth < 1 || CharWidth > 100)
            notify(hwnd, "CharWidth is %d!", CharWidth);
        WinReleasePS(hps);

        wordfile = fopen("TYPEFAST.DAT", "rb");
        if (wordfile == NULL) {
            notify(hwnd, "Cannot find TYPEFAST.DAT");
            WinPostMsg(hwnd, WM_QUIT, (MPARAM)0, (MPARAM)0);
        }

        kbbuf[0] = 0;
        for (i = 0; i < 30; i++)
            words[i] = (char *)malloc(sizeof(CHAR) * 20);

        DosGetDateTime(&dt);
        srand(dt.hours * 100 + dt.minutes * 10 + dt.seconds);
        if (wordfile) fseek(wordfile, (LONG)rand() % 10240L, SEEK_CUR);

        hwndMenu = WinWindowFromID(WinQueryWindow(hwnd, QW_PARENT), FID_MENU);

        /* Apply saved settings to menu checkmarks */
        apply_diff_check(hwndMenu, IDM_EASY, curDiff);
        apply_checked(hwndMenu, IDM_SOUND,      sound);
        apply_checked(hwndMenu, IDM_CASE,        respectCase);
        apply_checked(hwndMenu, IDM_BACKGRND,    bBackgrndRun);
        apply_checked(hwndMenu, IDM_SAVEONEXIT,  bSaveOnexit);
        apply_checked(hwndMenu, IDM_LANG_EN + iLang, TRUE);
        if (iLang != LANG_EN)
            apply_checked(hwndMenu, IDM_LANG_EN, FALSE);

        set_menu_lang(hwndMenu, iLang);

        return 0;
    }

    case WM_COMMAND:
    {
        USHORT command = SHORT1FROMMP(mp1);
        switch (command)
        {
        case IDM_NEWGAME:
            init();
            WinStartTimer(hab, hwnd, ID_TIMER, timertime);
            WinInvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case IDM_PAUSE:
            pausemode = !pausemode;
            apply_checked(hwndMenu, IDM_PAUSE, pausemode);
            return 0;

        case IDM_QUITGAME:
        {
            INT i;
            gameover = 1;
            WinStopTimer(hab, hwnd, ID_TIMER);
            for (i = 0; i < 30; i++) wordx[i] = 0;
            activewords = 0;
            WinInvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case IDM_EXIT:
            WinPostMsg(hwnd, WM_CLOSE, (MPARAM)0, (MPARAM)0);
            return 0;

        case IDM_EASY:
        case IDM_MEDIUM:
        case IDM_HARD:
        {
            INT prev = curDiff;
            INT next = (INT)command;
            INT newtime;
            switch (next) {
            case IDM_MEDIUM: newtime = TIMER_MEDIUM; break;
            case IDM_HARD:   newtime = TIMER_HARD;   break;
            default:         newtime = TIMER_EASY;   break;
            }
            apply_diff_check(hwndMenu, prev, next);
            curDiff   = next;
            maxactivewords = 4;
            timertime = newtime;
            WinStartTimer(hab, hwnd, ID_TIMER, timertime);
            return 0;
        }

        case IDM_SOUND:
            sound = !sound;
            apply_checked(hwndMenu, IDM_SOUND, sound);
            return 0;

        case IDM_CASE:
            respectCase = !respectCase;
            apply_checked(hwndMenu, IDM_CASE, respectCase);
            return 0;

        case IDM_BACKGRND:
            bBackgrndRun = !bBackgrndRun;
            apply_checked(hwndMenu, IDM_BACKGRND, bBackgrndRun);
            return 0;

        case IDM_FRAME:
            bFrameVisible = !bFrameVisible;
            frame_show(hwnd, bFrameVisible);
            /* checkmark when frame is HIDDEN */
            apply_checked(hwndMenu, IDM_FRAME, !bFrameVisible);
            return 0;

        case IDM_SAVEONEXIT:
            bSaveOnexit = !bSaveOnexit;
            apply_checked(hwndMenu, IDM_SAVEONEXIT, bSaveOnexit);
            return 0;

        case IDM_LANG_EN:
        case IDM_LANG_ES:
        case IDM_LANG_NL:
        case IDM_LANG_DE:
        case IDM_LANG_FR:
        case IDM_LANG_IT:
        {
            INT prev = IDM_LANG_EN + iLang;
            INT next = (INT)command;
            apply_diff_check(hwndMenu, prev, next);
            iLang = next - IDM_LANG_EN;
            set_menu_lang(hwndMenu, iLang);
            return 0;
        }

        case IDM_ABOUT:
            WinDlgBox(HWND_DESKTOP, hwnd,
                      (PFNWP)AboutDlgProc, NULLHANDLE, IDD_ABOUT, NULL);
            return 0;

        default:
            return WinDefWindowProc(hwnd, msg, mp1, mp2);
        }
        return 0;
    }

    case WM_SIZE:
        topx = SHORT1FROMMP(mp2);
        topy = SHORT2FROMMP(mp2);
        return 0;

    case WM_ERASEBACKGROUND:
        return (MRESULT)(TRUE);

    case WM_CHAR:
    {
        CHAR keycode, skeycode[2];
        INT i;

        if (!(CHARMSG(&msg)->fs & KC_CHAR)) return 0;
        if (CHARMSG(&msg)->fs & KC_KEYUP)   return 0;

        if (!anykeyhit) {
            DosGetDateTime(&dt);
            startsec = (float)(dt.hours * 3600 + dt.minutes * 60 + dt.seconds);
            anykeyhit = 1;
        }

        keycode = CHARMSG(&msg)->chr;
        if (keycode == 13 || keycode == ' ') {
            for (i = 0; i < 30; i++) {
                if (!wordx[i]) continue;
                if ((!respectCase && !stricmp(kbbuf, words[i])) ||
                    ( respectCase && !strcmp (kbbuf, words[i]))) {
                    score++;
                    activewords--;
                    wordx[i] = 0;
                    if (sound) DosBeep(2000, 40);
                    wordcountcorr++;
                    if ((score % 20) == 0) {
                        if (maxactivewords < 29) maxactivewords++;
                        if (score % 50 == 0) {
                            timertime -= 20;
                            maxactivewords /= 2;
                            if (timertime < 50) timertime = 50;
                            WinStartTimer(hab, hwnd, ID_TIMER, timertime);
                            if (sound) DosBeep(4000, 40);
                        }
                    }
                    break;
                }
            }
            wordcount++;
            kbbuf[0] = 0;
            return 0;
        }
        skeycode[0] = keycode;
        skeycode[1] = 0;
        if (strlen(kbbuf) < sizeof(kbbuf) - 2)
            strcat(kbbuf, skeycode);
        return 0;
    }

    case WM_SETFOCUS:
        if (!bBackgrndRun)
            pausemode = SHORT1FROMMP(mp2) ? 0 : 1;
        return 0;

    case WM_TIMER:
    {
        INT i, wordsize;

        if (pausemode || gameover) break;

        if (activewords < 4 && maxactivewords > 5) {
            timertime -= 50;
            if (timertime < 51) timertime = 51;
            WinStartTimer(hab, hwnd, ID_TIMER, timertime);
        }

        if (activewords < maxactivewords) {
            for (i = 0; i < 30; i++)
                if (wordx[i] == 0) break;
            if (i < 30) {
                activewords++;
                wordx[i] = rand() % topx + 1;
                strcpy(words[i], findword());
                wordsize = CharWidth * ((INT)strlen(words[i]) + 1);
                if (wordx[i] + wordsize > topx) {
                    wordx[i] -= wordsize;
                    if (wordx[i] < 1) wordx[i] = 1;
                }
                wordy[i] = topy - 1;
            }
        }

        WinInvalidateRect(hwnd, NULL, FALSE);

        for (i = 0; i < 30; i++) {
            if (wordx[i] != 0) {
                wordy[i] -= CharHeight + 1;
                if (wordy[i] < CharHeight) {
                    wordx[i] = 0;
                    if (anykeyhit) {
                        lost++;
                        if (sound) DosBeep(400, 10);
                    }
                    activewords--;
                    if (lost > 14) {
                        gameover = 1;
                        break;
                    }
                }
            }
        }
        break;
    }

    case WM_PAINT:
    {
        HPS   hps;
        RECTL rc, rcl;
        POINTL pt2;
        INT   i;
        CHAR  buffer[80];
        DATETIME dt2;
        float elapsedsec;

        WinQueryWindowRect(hwnd, &rcl);
        topx = rcl.xRight;
        topy = rcl.yTop;

        hps = WinBeginPaint(hwnd, 0L, &rc);
        GpiErase(hps);
        GpiSetColor(hps, CLR_NEUTRAL);
        GpiSetBackColor(hps, CLR_BACKGROUND);
        GpiSetBackMix(hps, BM_OVERPAINT);

        if (gameover) {
            pt2.x = 10;
            pt2.y = topy - 60;
            DosGetDateTime(&dt2);
            WinStopTimer(hab, hwnd, ID_TIMER);
            elapsedsec = (float)(dt2.hours * 3600 + dt2.minutes * 60 + dt2.seconds) - startsec;
            if (elapsedsec <= 0) elapsedsec = 1;
            sprintf(buffer, "That's %d words. You got %d words right,", lost, score);
            GpiCharStringAt(hps, &pt2, (LONG)strlen(buffer), buffer);
            pt2.y -= CharHeight * 2;
            sprintf(buffer, "and your WPM=%3.1f, correct WPM=%3.1f",
                    (float)wordcount    / elapsedsec * 60.0f,
                    (float)wordcountcorr / elapsedsec * 60.0f);
            GpiCharStringAt(hps, &pt2, (LONG)strlen(buffer), buffer);
            pt2.y -= CharHeight * 2;
            strcpy(buffer, "Select Game > New Game for another round.");
            GpiCharStringAt(hps, &pt2, (LONG)strlen(buffer), buffer);
        } else {
            for (i = 0; i < 30; i++) {
                if (wordx[i] != 0) {
                    pt2.y = wordy[i];
                    pt2.x = wordx[i];
                    GpiCharStringAt(hps, &pt2, (LONG)strlen(words[i]), words[i]);
                }
            }
        }

        WinEndPaint(hps);
        return 0;
    }

    case WM_CLOSE:
        if (bSaveOnexit) save_settings();
        if (wordfile) { fclose(wordfile); wordfile = NULL; }
        WinPostMsg(hwnd, WM_QUIT, (MPARAM)0, (MPARAM)0);
        break;

    default:
        return WinDefWindowProc(hwnd, msg, mp1, mp2);
    }
    return (MRESULT)FALSE;
}

MRESULT EXPENTRY AboutDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    switch (msg) {
    case WM_COMMAND:
        switch (SHORT1FROMMP(mp1)) {
        case DID_OK:
        case DID_CANCEL:
            WinDismissDlg(hwnd, TRUE);
            return 0;
        }
        break;
    }
    return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

VOID AbortTypeFast(HWND hwndFrame, HWND hwndClient)
{
    PERRINFO pErrInfoBlk;
    PSZ      pszOffset;

    if (sound) DosBeep(100, 50);
    if ((pErrInfoBlk = WinGetErrorInfo(hab)) != (PERRINFO)NULL) {
        pszOffset = ((PSZ)pErrInfoBlk) + pErrInfoBlk->offaoffszMsg;
        pszErrMsg = ((PSZ)pErrInfoBlk) + *((PSHORT)pszOffset);
        if ((INT)hwndFrame && (INT)hwndClient)
            WinMessageBox(HWND_DESKTOP, hwndFrame, (PSZ)pszErrMsg,
                          "Error", MSGBOXID,
                          MB_MOVEABLE | MB_CUACRITICAL | MB_CANCEL);
        WinFreeErrorInfo(pErrInfoBlk);
    }
    WinPostMsg(hwndClient, WM_QUIT, (MPARAM)NULL, (MPARAM)NULL);
}

INT notify(HWND hwndFrame, char *fmt, ...)
{
    CHAR out[256];
    va_list args;
    va_start(args, fmt);
    vsprintf(out, fmt, args);
    va_end(args);
    if (sound) { DosBeep(1000, 100); DosBeep(2000, 100); DosBeep(3000, 100); }
    DosSleep(2000);
    WinMessageBox(HWND_DESKTOP, hwndFrame, (PSZ)out, "Oops!", MSGBOXID,
                  MB_MOVEABLE | MB_INFORMATION | MB_OK);
    return 0;
}

BOOL goodword(char *p)
{
    INT i;
    if (strlen(p) < 3) return 0;
    if (strlen(p) < 5 && rand() < 20000) return 0;
    for (i = 0; i < 30; i++)
        if (wordx[i] && stricmp(p, words[i]) == 0) return 0;
    while (*p) {
        if (ispunct(*p)) return 0;
        p++;
    }
    return 1;
}

CHAR *findword(void)
{
    static CHAR wordfound[40];
    CHAR buffer[255];
    INT i = 0;

    wordfound[0] = 0;
    while (!goodword(wordfound)) {
        readline(buffer);
        if (buffer[0] == 0) return wordfound;
        while (!isspace(*(buffer + i))) {
            i++;
            if (*(buffer + i) == '\0') { i = 0; readline(buffer); }
        }
        while (isspace(*(buffer + i))) {
            i++;
            if (*(buffer + i) == '\0') { i = 0; readline(buffer); }
        }
        sscanf(buffer + i, "%s", wordfound);
    }
    return wordfound;
}

INT readline(char *p)
{
    *p = 0;
    while (1) {
        fseek(wordfile, (LONG)rand() % 80L, SEEK_CUR);
        if (fgets(p, 250, wordfile) == NULL) {
            fclose(wordfile);
            wordfile = fopen("TYPEFAST.DAT", "rb");
            if (wordfile == NULL) return 1;
            fseek(wordfile, 1L, SEEK_SET);
            continue;
        }
        break;
    }
    return 0;
}
