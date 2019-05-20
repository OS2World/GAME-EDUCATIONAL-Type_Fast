/* TYPEFAST for PM is written by Turgut Kalfaoglu <turgut@frors12.bitnet> */
/* This code can be compiled with IBM C/SET 2 Compiler                    */


#define INCL_WIN
#define INCL_GPI
#define INCL_DOSDATETIME
#include <os2.h>                        /* PM header file               */

#include "typefast.h"                   /* Resource symbolic identifiers*/
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

MRESULT EXPENTRY MyWindowProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
CHAR *findword(void);
                                        /* Define parameters by type    */
HAB  hab;                               /* PM anchor block handle       */
PSZ  pszErrMsg;
POINTL pt;
CHAR *words[30], kbbuf[256];
INT wordx[30],wordy[30],activewords,lost,score,maxactivewords,timertime,difflevel;
LONG CharHeight, CharWidth,   topx,topy;
FILE *wordfile;
BOOL anykeyhit,  pausemode,   gameover, sound,lastwordheight=0, respectCase=0;
ULONG wordcount, wordcountcorr;
float startsec;
CHAR *copyright="TYPEFAST V1.1 (C)1992 Turgut Kalfaoglu <TURGUT@FRORS12.BITNET>";

INT main (VOID)
{
  HMQ  hmq;                             /* Message queue handle         */
  HWND hwndClient = NULLHANDLE;         /* Client area window handle    */
  HWND hwndFrame  = NULLHANDLE;         /* Frame window handle          */
  QMSG qmsg;                            /* Message from message queue   */
  ULONG flCreate;                       /* Window creation control flags*/

  init();

  if ((hab = WinInitialize(0)) == 0L) /* Initialize PM     */
     AbortTypeFast(hwndFrame, hwndClient); /* Terminate the application */

  if ((hmq = WinCreateMsgQueue( hab, 0 )) == 0L)/* Create a msg queue   */
     AbortTypeFast(hwndFrame, hwndClient); /* Terminate the application */

  if (!WinRegisterClass(                /* Register window class        */
     hab,                               /* Anchor block handle          */
     (PSZ)"MyWindow",                   /* Window class name            */
     (PFNWP)MyWindowProc,               /* Address of window procedure  */
     CS_SIZEREDRAW,                     /* Class style                  */
     0                                  /* No extra window words        */
     ))
     AbortTypeFast(hwndFrame, hwndClient); /* Terminate the application */

   flCreate = FCF_STANDARD;             /* Set frame control flags to   */
                                        /* standard                     */
                                 

  if ((hwndFrame = WinCreateStdWindow(
               HWND_DESKTOP,            /* Desktop window is parent     */
               WS_VISIBLE,              /* STD. window styles           */
               &flCreate,               /* Frame control flag           */
               "MyWindow",              /* Client window class name     */
               "TypeFast",              /* No window text               */
               0L,                      /* No special class style       */
               (HMODULE)0L,             /* Resource is in .EXE file     */
               ID_WINDOW,               /* Frame window identifier      */
               &hwndClient              /* Client window handle         */
               )) == 0L)
     AbortTypeFast(hwndFrame, hwndClient); /* Terminate the application */

  WinSetWindowText(hwndFrame, "TypeFast!");

  WinStartTimer(hab, hwndClient, 1, timertime);

/*
 * Get and dispatch messages from the application message queue
 * until WinGetMsg returns FALSE, indicating a WM_QUIT message.
 */

  while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
    WinDispatchMsg( hab, &qmsg );

  WinStopTimer(hab,hwndClient, 1);
  WinDestroyWindow(hwndFrame);           /* Tidy up...                   */
  WinDestroyMsgQueue( hmq );             /* Tidy up...                   */
  WinTerminate( hab );                   /* Terminate the application    */
} /* End of main */

/**************************************************************************
 *
 *  Name       : MyWindowProc
 *
 *  Description: The window procedure associated with the client area in
 *               the standard frame window. It processes all messages
 *               either sent or posted to the client area, depending on
 *               the message command and parameters.
 *
 *  Parameters :  hwnd = window handle
 *                msg = message code
 *                mp1 = first message parameter
 *                mp2 = second message parameter
 *
 *  Return     :  depends on message sent
 *
 *************************************************************************/
MRESULT EXPENTRY MyWindowProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  DATETIME dt;
  static HWND hwndMenu;

  switch( msg )
  {
    case WM_CREATE:
    {
      INT i;
      FONTMETRICS fm;
      HPS hps;

      /* Learn size of characters */
      hps = WinGetPS(hwnd);
      GpiQueryFontMetrics(hps,(LONG) sizeof fm, &fm);
      CharHeight = (INT) fm.lMaxBaselineExt;
      CharWidth  = (INT) fm.lMaxCharInc;
      if (CharHeight < 1 | CharHeight > 50)
        notify(hwnd,"CharHeight is %d!",CharHeight);
      if (CharWidth < 1 | CharWidth > 100)
        notify(hwnd,"CharWidth is %d!",CharWidth);
      WinReleasePS(hps);
 
      wordfile = fopen("TYPEFAST.DAT","rb");
      if (wordfile == NULL) {
         notify(hwnd,"Cannot find TYPEFAST.DAT");
         WinPostMsg( hwnd, WM_QUIT, (MPARAM)0,(MPARAM)0 );
      }

      kbbuf[0] = 0; /* init keyb buffer */
      for (i=0;i<30;i++)
         words[i] = (char *) malloc(sizeof(CHAR)*20);

      DosGetDateTime(&dt);
      srand(dt.hours*100+dt.minutes*10+dt.seconds);
      fseek (wordfile,(LONG) rand()%10240L, SEEK_CUR);

      hwndMenu = WinWindowFromID(WinQueryWindow(hwnd,QW_PARENT),FID_MENU);
      difflevel = ID_EASY;
      return 0;
    }

    case WM_COMMAND:
      {
      USHORT command;                   /* WM_COMMAND command value     */
      command = SHORT1FROMMP(mp1);      /* Extract the command value    */
      switch (command)
      {    
      case ID_EASY:
      case ID_MEDIUM:
      case ID_HARD:

        /* remove check mark */
          WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(difflevel,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,0));
          difflevel = COMMANDMSG(&msg)->cmd;
          maxactivewords = 4;
          timertime = difflevel;
          WinStartTimer(hab, hwnd, 1, timertime);
          WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(difflevel,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
          return 0;

       case ID_ABOUT:
          WinMessageBox(HWND_DESKTOP, 
             hwnd,            /* Owner window is our frame */
             "TypeFast allows you to practice your typing by providing you"
             " with words of your choice (simply provide a text file to the"
             " program by naming it TYPEFAST.DAT), and measures the time it"
             " takes you to type words. You can use SPACE or RETURN between"
             " words. The program stays in demo mode until you hit a key.\n"
             "TypeFast for PM is written by Turgut Kalfaoglu, <turgut@frmop11.bitnet>\n"
             "Shareware: $10 donation requested if you find it useful."
             " Send donation to: Turgut Kalfaoglu, 1378 Sok 8/10, Izmir Turkey\n"
             " Thank you for trying TYPEFAST!",
             "About TypeFast",
             MSGBOXID,           
             MB_ICONEXCLAMATION | MB_OK ); /* Flags */
          return 0;

       case ID_RESTART: 
       {
          INT keepdiff, keepsound;
          keepdiff = difflevel;
          keepsound = sound;
          init();
          difflevel = keepdiff;
          sound  = keepsound;
          timertime = difflevel;
          WinStartTimer(hab,hwnd,1,timertime);
          return 0;
        }

        case ID_SOUND:
          sound = !sound;
          if (!sound) /* remove tick (check) mark */
             WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(ID_SOUND,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,FALSE));
          else        /* put tick (check) mark    */
             WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(ID_SOUND,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
          return 0;

        case ID_CASE:
          respectCase = !respectCase;
          if (!respectCase)
             WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(ID_CASE,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,FALSE));
          else
             WinSendMsg(hwndMenu,MM_SETITEMATTR,
                        MPFROM2SHORT(ID_CASE,TRUE),
                        MPFROM2SHORT(MIA_CHECKED,MIA_CHECKED));
          return 0;

        case ID_EXITPROG:
          WinPostMsg( hwnd, WM_CLOSE, (MPARAM)0, (MPARAM)0 );
          return 0;

        default:
          return WinDefWindowProc( hwnd, msg, mp1, mp2 );
      }
      return 0;
    }
    case WM_SIZE:
        topx = SHORT1FROMMP(mp2);
        topy = SHORT2FROMMP(mp2); 
        return 0;
 
    case WM_ERASEBACKGROUND:
      /*
       * Return TRUE to request PM to paint the window background
       * in SYSCLR_WINDOW.
       */
      return (MRESULT)( TRUE );

    case WM_CHAR:
    {
        CHAR keycode, skeycode[2];
        INT i;

        if (!(CHARMSG(&msg)->fs & KC_CHAR)) return 0; /* we're not interested */
        if (CHARMSG(&msg)->fs & KC_KEYUP) return 0;

        if (!anykeyhit) {
           i = DosGetDateTime(&dt);
           startsec = (float) (dt.hours*3600 + dt.minutes*60 + dt.seconds);
           anykeyhit = 1;
        } /* endif */

        keycode = CHARMSG(&msg)->chr;
        if ((keycode == 13) || (keycode == ' ')) { /* end of word? */
           for (i=0;i<30;i++) {
                if (!wordx[i]) continue;
                if (  !respectCase && !stricmp(kbbuf,words[i]) ||  /* hit! */
                       respectCase && !strcmp (kbbuf,words[i])) {
                   score++;
                   activewords--;
                   wordx[i] = 0;
                   if (sound) DosBeep(2000,40);
                   wordcountcorr++;
                   if ( (score%20)==0) {
                      if (maxactivewords<29)
                         maxactivewords++;
                      if (score%50==0) {
                         timertime -= 20;
                         maxactivewords /= 2;
                         if (timertime < 50) timertime = 50;
                         WinStartTimer(hab, hwnd, 1, timertime);
                         if (sound) DosBeep(4000,40);
                      }
                   }
                break;
                }/* hit */
           }
           wordcount++;
           kbbuf[0]=0;
           return 0;
        } /* user hit space or c/r */
        skeycode[0] = keycode;
        skeycode[1] = 0;
        strcat(kbbuf,skeycode);
        return 0;
    }

    case WM_SETFOCUS:
        if (SHORT1FROMMP(mp2)) /* if we got focus */
                pausemode = 0;
        else
                pausemode = 1;
        return 0;

    case WM_TIMER:
    {
        HPS hps;
        RECTL  rc;   
        INT i,wordsize;

        if (pausemode || gameover) break;
        if (activewords < 4 && maxactivewords>5) {
              timertime -= 50;
              if (timertime<51) timertime=51;
              WinStartTimer(hab, hwnd, 1, timertime);
        }
        if (activewords < maxactivewords) {
           for (i=0;i<30;i++)
               if (wordx[i] == 0) break;
        /* Creating a new word.. */
           if (i<30) {
              activewords++;
              wordx[i] = rand() % topx + 1;
              strcpy(words[i],findword());
              wordsize = CharWidth*(strlen(words[i])+1);
              if (wordx[i]+wordsize > topx ) {
                  wordx[i] -= wordsize;
                  if (wordx[i]<1) wordx[i]=1;
              }
              wordy[i] = topy-1;
           }
        }
        WinInvalidateRect(hwnd,NULL,FALSE);

        /* move down everyone by one */
        for (i=0;i<30;i++) {
            if (wordx[i] !=0) {
                pt.x = wordx[i];
                pt.y = wordy[i];
                /* move down */
                wordy[i] -= CharHeight+1;
                /* reach the bottom? */
                if (wordy[i] < CharHeight) {
                   wordx[i] = 0;
                   if (anykeyhit) {
                        lost++;
                        if (sound) DosBeep(400,10); }
                   activewords--;
                   if (lost>14) {
                      gameover = 1;
                      break;
                   }
                }
                pt.y = wordy[i];
            }
        }
        break;
    }
 
    case WM_PAINT:
      /*
       * Window contents are drawn here in WM_PAINT processing.
       */
      {
      HPS    hps;                       /* Presentation Space handle    */
      RECTL  rc,rcl;
      POINTL pt;                        /* String screen coordinates    */
                                        /* Create a presentation space  */
      CHAR dest;
      INT i;
      CHAR buffer[80];
      DATETIME dt;
      float elapsedsec;

      WinQueryWindowRect(hwnd, &rcl);
      topx = rcl.xRight;
      topy = rcl.yTop; 


      hps = WinBeginPaint( hwnd, 0L, &rc );
      GpiErase(hps);
      GpiSetColor( hps, CLR_NEUTRAL );         /* colour of the text,   */
      GpiSetBackColor( hps, CLR_BACKGROUND );  /* its background and    */
      GpiSetBackMix( hps, BM_OVERPAINT );      /* how it mixes,         */
                                               /* and draw the string...*/
      if (gameover) {
         pt.x = 10;
         pt.y = topy - 60;
         DosGetDateTime(&dt);
         WinStopTimer(hab, hwnd, 1);
         elapsedsec = (float) (dt.hours*3600+dt.minutes*60+dt.seconds) - startsec;
         if (elapsedsec == 0L) elapsedsec = 1L;
         sprintf(buffer,"That's %d words. You got %d words right, ",lost,score);
         GpiCharStringAt(hps,&pt,(LONG)strlen(buffer),buffer);
         pt.y -= CharHeight*2;
         sprintf(buffer,"and your WPM=%3.1f, correct WPM=%3.1f",
            (float) wordcount / elapsedsec * 60.0,
            (float) wordcountcorr / elapsedsec * 60.0);
         GpiCharStringAt(hps,&pt,(LONG)strlen(buffer),buffer);
         pt.y -= CharHeight*2;
         strcpy(buffer,"Select \"Restart\" for another game.");
         GpiCharStringAt(hps,&pt,(LONG)strlen(buffer),buffer);
      }
      else
        for (i=0;i<30;i++)
          if (wordx[i] != 0) {
                pt.y = wordy[i];
                pt.x = wordx[i];
                GpiCharStringAt( hps, &pt, (LONG)strlen(words[i]), words[i]);
          }

      WinEndPaint( hps );                      /* Drawing is complete   */
      return 0;
      }

    case WM_CLOSE:
      /*
       * This is the place to put your termination routines
       */
      fclose(wordfile);
      WinPostMsg( hwnd, WM_QUIT, (MPARAM)0,(MPARAM)0 );/* Cause termination*/
      break;

    default:
      /*
       * Everything else comes here.  This call MUST exist
       * in your window procedure.
       */

      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }
  return (MRESULT)FALSE;
} /* End of MyWindowProc */

/**************************************************************************
 *
 *  Name       : AbortTypeFast
 *
 *  Description: Report an error returned from an API service
 *
 *  Concepts   :  use of message box to display information
 *
 *  API's      :  DosBeep
 *                WinGetErrorInfo
 *                WinMessageBox
 *                WinFreeErrorInfo
 *                WinPostMsg
 *
 *  Parameters :  hwndFrame = frame window handle
 *                hwndClient = client window handle
 *
 *  Return     :  [none]
 *
 *************************************************************************/
VOID AbortTypeFast(HWND hwndFrame, HWND hwndClient)
{
   PERRINFO  pErrInfoBlk;
   PSZ       pszOffSet;
   void      stdprint(void);

   if (sound) DosBeep(100,50);
   if ((pErrInfoBlk = WinGetErrorInfo(hab)) != (PERRINFO)NULL)
   {
      pszOffSet = ((PSZ)pErrInfoBlk) + pErrInfoBlk->offaoffszMsg;
      pszErrMsg = ((PSZ)pErrInfoBlk) + *((PSHORT)pszOffSet);
      if((INT)hwndFrame && (INT)hwndClient)
         WinMessageBox(HWND_DESKTOP,         /* Parent window is desk top */
                       hwndFrame,            /* Owner window is our frame */
                       (PSZ)pszErrMsg,       /* PMWIN Error message       */
                       "Error Msg",          /* Title bar message         */
                       MSGBOXID,             /* Message identifier        */
                       MB_MOVEABLE | MB_CUACRITICAL | MB_CANCEL ); /* Flags */
      WinFreeErrorInfo(pErrInfoBlk);
   }
   WinPostMsg(hwndClient, WM_QUIT, (MPARAM)NULL, (MPARAM)NULL);
} /* End of AbortTypeFast */


INT notify(HWND hwndFrame,char *blurb, INT code)
{
   CHAR out[80];

   sprintf(out,blurb,code);
   if (sound) DosBeep(1000,100);
   if (sound) DosBeep(2000,100);
   if (sound) DosBeep(3000,100);
   DosSleep(2000);
   WinMessageBox(HWND_DESKTOP,
                 hwndFrame,
                (PSZ) out,
                 "Oops!",
                 MSGBOXID,
                 MB_MOVEABLE | MB_INFORMATION | MB_OK ); /* Flags */
}

BOOL goodword(char *p)
{
   INT i;
        /* we admit short words, but not too often */
        if (strlen(p)<3) return 0; /* not too short */
        if (strlen(p)<5 && rand()<20000) return 0;
        for (i=0;i<30;i++)
           if (wordx[i] && stricmp(p,words[i])==0) return 0;

        while (*p) {
                if (ispunct(*p)) return 0;
                p++; 
        }
        return 1;
}

CHAR *findword() {
   static CHAR wordfound[40];
   CHAR buffer[255];
   INT i=0;

   wordfound[0]=0;
   while (!goodword(wordfound)) {
      readline(buffer);
      if (buffer[0] == 0) return NULL; /* error reading  */
      while (!isspace(*(buffer+i))) {  /* find a space.. */
        i++;
        if (*(buffer+i) == '\0') {
           i=0;
           readline(buffer);
        }
      }
      while (isspace(*(buffer+i))) { /* find a non-space */
        i++;
        if (*(buffer+i) == '\0') {
                i=0;
                readline(buffer); 
        }
      }
      sscanf(buffer+i,"%s",wordfound);
   }
   return wordfound;
}
INT readline(char *p)
{
      *p = 0;
      while (1) {
              fseek (wordfile,(LONG) rand()%80L, SEEK_CUR);
              if (fgets(p,250,wordfile) == NULL) {
                fclose(wordfile);
                wordfile = fopen("TYPEFAST.DAT","rb");
                if (wordfile == NULL) return 1;
                fseek(wordfile,1L,SEEK_SET);
                continue; }
              break;
      }
}

INT init() {
   INT i;

   activewords = 0;
   lost = 0;   score = 0;  maxactivewords=4; timertime=ID_EASY;
   topx = 400L; topy = 400L; anykeyhit=0; pausemode=0; gameover=0;
   wordcount=0L; wordcountcorr=0L; lastwordheight=0;
   kbbuf[0]=0; sound=1;
   for(i=0;i<30;i++)
      wordx[i]=0;
}

