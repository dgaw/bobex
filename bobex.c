#include <exec/memory.h>
#include <exec/types.h>
#include <graphics/gels.h>
#include <libraries/dos.h>

#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#include <stdio.h>

#include "animtools/animtools.h"
#include "animtools/animtools_proto.h"

VOID bobDrawGList(struct RastPort *rport, struct ViewPort *vport);
void processWindow(struct Window *win, struct Bob *myBob);
void doBob(struct Window *win);

struct GfxBase *GfxBase;             /* pointer to Graphics library */
struct IntuitionBase *IntuitionBase; /* pointer to Intuition library */

int returnCode;

/* number of lines in the bob */
#define GEL_SIZE 4

/* 16x4 pixels, 2 planes */
USHORT chip bobData[8] =
{
  0xeeee, 0xeeee, 0x0000, 0xeeee,
  0xbbbb, 0xbbbb, 0xffff, 0xbbbb,
};


/* 32x4 pixels, 2 planes */
/*
USHORT chip bobData[16] =
{
  0xeeee, 0xeeee, 0xeeee, 0xeeee, 0x0000, 0x0000, 0xeeee, 0xeeee,
  0xbbbb, 0xbbbb, 0xbbbb, 0xbbbb, 0xffff, 0xffff, 0xbbbb, 0xbbbb,
};
*/

/* information for the new bob
** structure defined in animtools.h
*/
NEWBOB myNewBob = {
    bobData,   /* image data               */
    1,         /* WORD width               */
    GEL_SIZE,  /* line height              */
    2,         /* image depth              */
    3,
    0,                  /* plane pick, plane on off */
    SAVEBACK | OVERLAY, /* vsprite flags            */
    0,                  /* dbuf (0 == false)        */
    2,                  /* raster depth             */
    160,
    100, /* x,y position             */
    0,
    0, /* hit mask, me mask        */
};

#define SBMWIDTH 320 /* My screen size constants. */
#define SBMHEIGHT 200
#define SBMDEPTH 4
#define SCRNMODE NULL /* (HIRES | LACE) for NewScreen, ends up in view.*/

struct NewScreen ns = {0,         0,        SBMWIDTH,
                       SBMHEIGHT, SBMDEPTH, 0,
                       0,         SCRNMODE, CUSTOMSCREEN | SCREENQUIET,
                       NULL,      NULL,     NULL,
                       NULL};

/* information for the new window */
struct NewWindow myNewWindow = {0,
                                0,
                                SBMWIDTH,
                                SBMHEIGHT,
                                0,
                                0,
                                CLOSEWINDOW,
                                WINDOWCLOSE | RMBTRAP | ACTIVATE,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                0,
                                0,
                                SBMWIDTH,
                                SBMHEIGHT - 1,
                                CUSTOMSCREEN};

/*-------------------------------------------------------------
** draw the bobs into the rast port.
*/
VOID bobDrawGList(struct RastPort *rport, struct ViewPort *vport) {
  /* only need to MrgCop() LoadView() if using TRUE VSprites */
  SortGList(rport);
  DrawGList(rport, vport);
  WaitTOF();
}

/*-------------------------------------------------------------
** process window and dynamically change bob:
** - get messages.
** - go away on CLOSEWINDOW.
** - update and redisplay bob on INTUITICKS.
** - wait for more messages.
*/
void processWindow(struct Window *win, struct Bob *myBob) {
  struct IntuiMessage *msg;

  FOREVER {
    Wait(1L << win->UserPort->mp_SigBit);

    while (NULL != (msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
      /* only CLOSEWINDOW is active */
      if (msg->Class == CLOSEWINDOW) {
        ReplyMsg((struct Message *)msg);
        return;
      }
    }

    bobDrawGList(win->RPort, ViewPortAddress(win));
  }
}

/*-------------------------------------------------------------
** working with the bob:
** - setup the gel system, and get a new bob (makeBob()).
** - add the bob to the system and display.
** - use the bob.
** - when done, remove the bob and update the display without the bob.
** - cleanup everything.
*/
void doBob(struct Window *win) {
  struct Bob *myBob;
  struct GelsInfo *my_ginfo;

  if (NULL == (my_ginfo = setupGelSys(win->RPort, (UBYTE)0xfc))) {
    printf("Error: Can't setup GEL system\n");
    returnCode = RETURN_WARN;
  } else {
    if (NULL == (myBob = makeBob(&myNewBob))) {
      printf("Error: Can't make bob \n");
      returnCode = RETURN_WARN;
    } else {
      AddBob(myBob, win->RPort);
      bobDrawGList(win->RPort, ViewPortAddress(win));

      processWindow(win, myBob);

      RemBob(myBob);
      bobDrawGList(win->RPort, ViewPortAddress(win));

      freeBob(myBob, myNewBob.nb_RasDepth);
    }
    cleanupGelSys(my_ginfo, win->RPort);
  }
}

/*-------------------------------------------------------------
** example bob program:
** - first open up the libraries and a window.
*/
int main(int argc, char **argv) {
  struct Window *win;
  struct Screen *screen;

  returnCode = RETURN_OK;

  GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 33L);
  if (GfxBase == NULL) {
    printf("Error: can't open graphics.library\n");
    returnCode = RETURN_FAIL;
  } else {
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 33L);
    if (IntuitionBase == NULL) {
      printf("Error: can't open intuition.library\n");
      returnCode = RETURN_FAIL;
    } else {
      screen = (struct Screen *)OpenScreen(&ns);
      if (screen == NULL) {
        printf("Error: can't open screen\n");
        returnCode = RETURN_FAIL;
      } else {
        myNewWindow.Screen = screen;
        win = OpenWindow(&myNewWindow);
        if (win == NULL) {
          printf("Error: can't open window\n");
          returnCode = RETURN_FAIL;
        } else {
          doBob(win);
          CloseWindow(win);
        }
        CloseScreen(screen);
      }
      CloseLibrary((struct Library *)IntuitionBase);
    }
    CloseLibrary((struct Library *)GfxBase);
  }
  return returnCode;
}
