#include "player.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	// Main
	WNDCLASS w;
	regClass(w, szProgName, WndProc);
	// Buttons
	WNDCLASS playClass;
	regClass(playClass, L"play", customButtonProc);

	WNDCLASS pauseClass;
	regClass(pauseClass, L"pause", customButtonProc);

	WNDCLASS closeClass;
	regClass(closeClass, L"close", customButtonProc);

	WNDCLASS nextClass;
	regClass(nextClass, L"next", customButtonProc);

	WNDCLASS prevClass;
	regClass(prevClass, L"prev", customButtonProc);

	if (!(RegisterClass(&w) && RegisterClass(&playClass) && RegisterClass(&pauseClass)
		&& RegisterClass(&closeClass) && RegisterClass(&nextClass) && RegisterClass(&prevClass))) {
		MessageBox(NULL, L"Cannot register class", L"Error", MB_OK);
		return NULL;
	}

	mainBmp = (HBITMAP)LoadImage(NULL, mainBmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	mainMask = (HBITMAP)LoadImage(NULL, mainMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	pauseMask = (HBITMAP)LoadImage(NULL, pauseMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	playMask = (HBITMAP)LoadImage(NULL, playMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	closeMask = (HBITMAP)LoadImage(NULL, closeMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	GetObject(mainMask, sizeof(BITMAP), &bi);

	if (!mainMask || !playMask || !pauseMask || !closeMask || !mainBmp) {
		MessageBox(hWnd, L"Cannot find mask", L"Missing file", MB_OK);
		return NULL;
	}

	if (!BASS_Init(-1, // use the default device
		44100, // sample rate in Hertz for the output mixer
		0, 0, NULL)) {
		MessageBox(hWnd, L"Cannot init Bass library", L"Load library", MB_OK);
		return NULL;
	}

	hWnd = CreateWindow(szProgName,		// Class name
		L"Audio Player",				// window name
		WS_TILEDWINDOW,					// overlapped window
		CW_USEDEFAULT, CW_USEDEFAULT,	// position of the window
		bi.bmWidth, bi.bmHeight,		// width and height
		NULL,							// parent window
		NULL,							// no menu
		hInstance, NULL);
	if (!hWnd) {
		MessageBox(NULL, L"Cannot create window", L"Error", MB_OK);
		return NULL;
	}

	//somehow, it is necessary that the window is not clipped
	int Style; 
	Style = GetWindowLong(hWnd, GWL_STYLE);
		Style = Style || WS_CAPTION;
		Style = Style || WS_SYSMENU;
	SetWindowLong(hWnd, GWL_STYLE, Style);

	// Show window, start handling his messages
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DestroyWindow(hWnd);
	UnregisterClass(szProgName, NULL);
	return msg.wParam;
}

HRGN createRgnFromBitmap(HBITMAP mask) {
	BYTE bpp;
	DWORD TransPixel;
	DWORD pixel;
	int startx;
	INT i, j;
	HRGN Rgn, ResRgn = CreateRectRgn(0, 0, 0, 0);
	BITMAP bm;
	GetObject(mask, sizeof(BITMAP), &bm);
	bpp = bm.bmBitsPixel >> 3; // number of bytes required to indicate the color of a pixel
	BYTE *pBits = new BYTE[bm.bmWidth * bm.bmHeight * bpp];

	int  p = GetBitmapBits(mask, bm.bmWidth * bm.bmHeight * bpp, pBits);

	TransPixel = *(DWORD*)pBits; // color 1 pixel

	TransPixel <<= (32 - bm.bmBitsPixel);
	// line by line, draw
	for (i = 0; i < bm.bmHeight; i++){
		startx = -1;	// start new line
		for (j = 0; j < bm.bmWidth; j++){ // get pixel
			pixel = *(DWORD*)(pBits + (i * bm.bmWidth +
				j) * bpp) << (32 - bm.bmBitsPixel);
			if ((pixel != TransPixel) && (startx<0)) {
				startx = j; // start of the displayed line 
			}
			if (((pixel != TransPixel) && (j == (bm.bmWidth - 1))) || // reached the border of the picture
				((pixel == TransPixel) && (startx >= 0))) {  // reached the end of the displayed line
				Rgn = CreateRectRgn(startx, i, j, i + 1);
				CombineRgn(ResRgn, ResRgn, Rgn, RGN_OR);
				startx = -1;
			}
		}
	}
	delete pBits;
	return ResRgn;
}

void CALLBACK UpdateEqualizer(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2){
	HDC dc;
	int x, y, y1;

	float fft[1024];
	BASS_ChannelGetData(streamHandle, fft, BASS_DATA_FFT2048);

	int b0 = 0;
	memset(specbuf, 0, visWidth*visHeight);
	for (x = 0; x<BANDS; x++) {
		float peak = 0;
		int b1 = pow(2, x*10.0 / (BANDS - 1));
		if (b1 <= b0) b1 = b0 + 1;
		if (b1>1023) b1 = 1023;
		for (; b0<b1; b0++)
			if (peak<fft[1 + b0]) peak = fft[1 + b0];
		y = sqrt(peak) * 3 * visHeight - 4;
		if (y>visHeight) y = visHeight;
		while (--y >= 0)
			memset(specbuf + y*visWidth + x*(visWidth / BANDS), y + 1, visWidth / BANDS - 2);
	}

	dc = GetDC(hWnd);
	BitBlt(dc, visPosLeft, visPosTop, visWidth, visHeight, specDC, 0, 0, SRCCOPY);
	ReleaseDC(hWnd, dc);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	PAINTSTRUCT ps;
	HDC hdc;
	HDC hdcBits;
	RECT wndrect;
	POINT point;
	switch (msg) {
	case WM_CREATE:
		SetWindowRgn(hWnd, createRgnFromBitmap(mainMask), TRUE);
		playButton = CreateWindow(L"play", 0,
			WS_CHILD | BS_BITMAP | BS_OWNERDRAW,
			103, 120, 75, 75, hWnd, (HMENU)ID_BUTTON_PLAY, NULL, NULL);
		SetWindowRgn(playButton, createRgnFromBitmap(playMask), TRUE);
		pauseButton = CreateWindow(L"pause", 0,
			WS_CHILD | BS_BITMAP | BS_OWNERDRAW,
			103, 120, 75, 75, hWnd, (HMENU)ID_BUTTON_PAUSE, NULL, NULL);
		SetWindowRgn(pauseButton, createRgnFromBitmap(pauseMask), TRUE);
		closeButton = CreateWindow(L"close", 0,
			WS_CHILD | BS_BITMAP | BS_OWNERDRAW,
			494, 120, 75, 75, hWnd, (HMENU)ID_BUTTON_EXIT, NULL, NULL);
		SetWindowRgn(closeButton, createRgnFromBitmap(closeMask), TRUE);
		streamHandle = BASS_StreamCreateFile(FALSE, sound, 0, 0, BASS_SAMPLE_LOOP);

		// color equalizer
		{
			BYTE data[2000] = { 0 };
			BITMAPINFOHEADER *bh = (BITMAPINFOHEADER*)data;
			RGBQUAD *pal = (RGBQUAD*)(data + sizeof(*bh));
			int a;
			bh->biSize = sizeof(*bh);
			bh->biWidth = visWidth;
			bh->biHeight = visHeight;
			bh->biPlanes = 1;
			bh->biBitCount = 8;
			bh->biClrUsed = bh->biClrImportant = 256;

			for (a = 1; a<128; a++) {
				pal[a].rgbRed = 256 - 2 * a;
				pal[a].rgbGreen = 2 * a;
			}
			for (a = 0; a<32; a++) {
				pal[128 + a].rgbBlue = 8 * a;
				pal[128 + 32 + a].rgbBlue = 255;
				pal[128 + 32 + a].rgbGreen = 8 * a;
				pal[128 + 64 + a].rgbGreen = 255;
				pal[128 + 64 + a].rgbBlue = 8 * (31 - a);
				pal[128 + 64 + a].rgbRed = 8 * a;
				pal[128 + 96 + a].rgbGreen = 255;
				pal[128 + 96 + a].rgbRed = 255;
				pal[128 + 96 + a].rgbBlue = 8 * a;
			}

			specbmp = CreateDIBSection(0, (BITMAPINFO*)bh, DIB_RGB_COLORS, (void**)&specbuf, NULL, 0);
			specDC = CreateCompatibleDC(0);
			SelectObject(specDC, specbmp);
		}
		break;
	case WM_PAINT:
		ShowWindow(closeButton, SW_SHOW);
		if (isPlayingNow){
			ShowWindow(pauseButton, SW_SHOW);
			ShowWindow(playButton, SW_HIDE);
			EnableWindow(pauseButton, true);
			EnableWindow(playButton, false);
			UpdateWindow(pauseButton);
		}
		else{
			ShowWindow(pauseButton, SW_HIDE);
			ShowWindow(playButton, SW_SHOW);
			EnableWindow(pauseButton, false);
			EnableWindow(playButton, true);
			UpdateWindow(playButton);
		}


		hdc = BeginPaint(hWnd, &ps);
		hdcBits = ::CreateCompatibleDC(hdc);
		SelectObject(hdcBits, mainBmp);
		BitBlt(hdc, 0, 0, bi.bmWidth, bi.bmHeight, hdcBits, 0, 0, SRCCOPY);
		DeleteDC(hdcBits);
		EndPaint(hWnd, &ps);
		break;

	case WM_LBUTTONDOWN:
		GetCursorPos(&MousePnt);
		dragging = true;
		SetCapture(hWnd);
		break;

	case WM_MOUSEMOVE:
		if (dragging){
			GetCursorPos(&point);
			GetWindowRect(hWnd, &wndrect);
			wndrect.left = wndrect.left + (point.x - MousePnt.x);
			wndrect.top = wndrect.top + (point.y - MousePnt.y);
			SetWindowPos(hWnd, NULL, wndrect.left, wndrect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			MousePnt = point;
		}
		break;
	case WM_LBUTTONUP:
		if (dragging){
			dragging = false;
			ReleaseCapture();
		}
		break;

	case WM_KEYUP:
		if (wp == VK_SPACE) {
			if (isPlayingNow){
				KillTimer(hWnd, 25);
				isPlayingNow = false;
				BASS_ChannelPause(streamHandle);
			}
			else {
				SetTimer(hWnd, 25, 25, (TIMERPROC)&UpdateEqualizer);
				BASS_ChannelPlay(streamHandle, FALSE);
				isPlayingNow = true;
			}
			SendMessage(hWnd, WM_PAINT, GetWindowLong(hWnd, GWL_ID), (LPARAM)hWnd);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wp)){
		case ID_BUTTON_PLAY:
			SetTimer(hWnd, 25, 25, (TIMERPROC)&UpdateEqualizer);
			BASS_ChannelPlay(streamHandle, FALSE);
			isPlayingNow = true;
			break;
		case ID_BUTTON_PAUSE:
			KillTimer(hWnd, 25);
			BASS_ChannelPause(streamHandle);
			isPlayingNow = false;
			break;
		case ID_BUTTON_EXIT:
			PostMessage(hWnd, WM_CLOSE, wp, lp);
			break;
		default:
			break;
		}

		break;
	case WM_DESTROY:
		BASS_Free();
		DeleteObject(mainMask);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}


LRESULT CALLBACK customButtonProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	HDC hdc, h;
	PAINTSTRUCT ps;
	BITMAP bm;
	HBITMAP mask;
	wchar_t text[maxCustomButtonClassName];
	switch (msg)
	{
	case WM_PAINT:
		RealGetWindowClass(hWnd, text, maxCustomButtonClassName);
		if (wcscmp(text, L"play") == 0){
			mask = (HBITMAP)LoadImage(NULL, playMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		}
		else if (wcscmp(text, L"paus") == 0){
			mask = (HBITMAP)LoadImage(NULL, pauseMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		}
		else if (wcscmp(text, L"clos") == 0){
			mask = (HBITMAP)LoadImage(NULL, closeMaskPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		}
		
		GetObject(mask, sizeof(BITMAP), &bm);
		hdc = BeginPaint(hWnd, &ps);
		h = ::CreateCompatibleDC(hdc);
		SelectObject(h, mask);
		BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, h, 0, 0, SRCCOPY);
		DeleteDC(h);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_LBUTTONUP:
		SendMessage(GetParent(hWnd), WM_COMMAND, GetWindowLong(hWnd, GWL_ID), (LPARAM)hWnd);
		SendMessage(GetParent(hWnd), WM_PAINT, GetWindowLong(hWnd, GWL_ID), (LPARAM)hWnd);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wp, lp);
}
