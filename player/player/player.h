#pragma once
#pragma comment(lib,"../SoundLib/c/bass.lib")

#include <Windows.h>
#include <conio.h>
#include <math.h>
#include "../SoundLib/c/bass.h"

#define ID_BUTTON_PLAY	1
#define ID_BUTTON_PAUSE	2
#define ID_BUTTON_EXIT	3
#define ID_BUTTON_PREV	4
#define ID_BUTTON_NEXT	5

// equalize
#define visWidth		116
#define visHeight		65
#define visPosLeft		275
#define visPosTop		465
#define BANDS 20

#define maxCustomButtonClassName 5

#define regClass(nameClass, classN,LWP){							\
	nameClass.lpszClassName = classN;								\
	nameClass.hInstance = hInstance;								\
	nameClass.lpfnWndProc = LWP;								\
	nameClass.hCursor = LoadCursor(NULL, IDC_ARROW);				\
	nameClass.hIcon = NULL;											\
	nameClass.lpszMenuName = NULL;									\
	nameClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);	\
	nameClass.style = CS_HREDRAW | CS_VREDRAW;						\
	nameClass.cbClsExtra = 0;										\
	nameClass.cbWndExtra = 0;										\
	}


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK customButtonProc(HWND, UINT, WPARAM, LPARAM);
void createButtons(HWND);
HRGN createRgnFromBitmap(HBITMAP);


HWND		hWnd, playButton, pauseButton, closeButton, nextButton, prevButton;
HDC			specDC;
HBITMAP		mainMask, mainBmp, pauseMask, playMask, closeMask, specbmp;
BITMAP		bi;

wchar_t		szProgName[] = L"Audio player";
wchar_t		sound[] = L"../Resources/audio/Bomba.mp3";
wchar_t		mainMaskPath[] = L"../Resources/bmp/kontrust_mask.bmp";
wchar_t		mainBmpPath[] = L"../Resources/bmp/kontrust.bmp";
wchar_t		playMaskPath[] = L"../Resources/bmp/play.bmp";
wchar_t		pauseMaskPath[] = L"../Resources/bmp/pause.bmp";
wchar_t		closeMaskPath[] = L"../Resources/bmp/close.bmp";
wchar_t		isPlayingNow = false;
wchar_t		dragging = false;
POINT		MousePnt;
HSTREAM		streamHandle;
BYTE		*specbuf;