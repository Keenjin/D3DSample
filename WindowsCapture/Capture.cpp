#include "Capture.h"
#include "framework.h"
#include "BitmapMemDC.h"

void SaveBitmapToFile(HBITMAP hBitMap, LPCWSTR lpstrFileName)
{
    BITMAP bitmap;
    GetObject(hBitMap, sizeof(BITMAP), &bitmap);

    BITMAPFILEHEADER       bmfHdr;           //λͼ�ļ�ͷ�ṹ
    BITMAPINFOHEADER       bi;               //λͼ��Ϣͷ�ṹ
    LPBITMAPINFOHEADER   lpbi;               //ָ��λͼ��Ϣͷ�ṹ

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bitmap.bmWidth;
    bi.biHeight = bitmap.bmHeight;
    bi.biPlanes = 1;
    HDC hDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);
    int iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    DeleteDC(hDC);
    if (iBits <= 1) bi.biBitCount = 1;
    else if (iBits <= 4) bi.biBitCount = 4;
    else if (iBits <= 8) bi.biBitCount = 8;
    else if (iBits <= 24) bi.biBitCount = 24;
    else bi.biBitCount = iBits;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmBitsSize = ((bitmap.bmWidth *
        bi.biBitCount + 31) / 32) * 4
        * bitmap.bmHeight;

    //�����ɫ���С
    DWORD dwPaletteSize = 0;
    if (bi.biBitCount <= 8)
        dwPaletteSize = (1 << bi.biBitCount) * sizeof(RGBQUAD);

    //   ����λͼ�ļ�ͷ
    bmfHdr.bfType = 0x4D42;     //   "BM "
    DWORD dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
        + dwPaletteSize + dwBmBitsSize;
    bmfHdr.bfSize = dwDIBSize;
    bmfHdr.bfReserved1 = 0;
    bmfHdr.bfReserved2 = 0;
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)
        + (DWORD)sizeof(BITMAPINFOHEADER)
        + dwPaletteSize;

    //Ϊλͼ���ݷ����ڴ�
    HANDLE hDib = GlobalAlloc(GHND, dwBmBitsSize +
        dwPaletteSize + sizeof(BITMAPINFOHEADER));
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
    *lpbi = bi;
    //   �����ɫ��      
    HPALETTE hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
    HPALETTE hOldPal = NULL;
    if (hPal)
    {
        hDC = GetDC(NULL);
        hOldPal = SelectPalette(hDC, hPal, FALSE);
        RealizePalette(hDC);
    }
    //   ��ȡ�õ�ɫ�����µ�����ֵ
    GetDIBits(hDC, hBitMap, 0, (UINT)bitmap.bmHeight,
        (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwPaletteSize, (LPBITMAPINFO)lpbi, DIB_RGB_COLORS);
    //�ָ���ɫ��      
    if (hOldPal)
    {
        SelectPalette(hDC, hOldPal, TRUE);
        RealizePalette(hDC);
        ReleaseDC(NULL, hDC);
    }

	HANDLE hFile = CreateFile(lpstrFileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    
	//   д��λͼ�ļ�ͷ
    DWORD dwWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	//   д��λͼ�ļ���������
	WriteFile(hFile, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL);
    GlobalUnlock(hDib);
    GlobalFree(hDib);
    CloseHandle(hFile);
}

void Capture(HWND hWnd)
{
    HDC hdc = GetDC(hWnd);

    RECT rcWnd = { 0 };
    GetClientRect(hWnd, &rcWnd);

    int cx = rcWnd.right - rcWnd.left;
    int cy = rcWnd.bottom - rcWnd.top;

    /*BITMAPINFO bi = { 0 };
    BITMAPINFOHEADER* bih = &bi.bmiHeader;
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biBitCount = 32;
    bih->biWidth = cx;
    bih->biHeight = cy;
    bih->biPlanes = 1;

    capture->hdc = CreateCompatibleDC(NULL);
    capture->bmp =
        CreateDIBSection(capture->hdc, &bi, DIB_RGB_COLORS,
            (void**)&capture->bits, NULL, 0);
    capture->old_bmp = SelectObject(capture->hdc, capture->bmp);*/

    CBitmapMemDC bitmap;
    bitmap.CreateBitmapFromDC(0, 0, cx, cy, hdc);

    //HDC hdcMem = CreateCompatibleDC(hdc);
    //HBITMAP bitmap = CreateCompatibleBitmap(hdc, cx, cy);
    //SelectObject(hdcMem, bitmap);
    //BitBlt(hdcMem, 0, 0, cx, cy, hdc, 0, 0, SRCCOPY);

    SaveBitmapToFile(bitmap, L"d:\\test.bmp");

    ReleaseDC(hWnd, hdc);
    //DeleteDC(hdcMem);
    //DeleteObject(bitmap);
}