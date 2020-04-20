// BitmapMemDC.cpp: implementation of the CBitmapMemDC class.
//
//////////////////////////////////////////////////////////////////////

#include "BitmapMemDC.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBitmapMemDC::CBitmapMemDC()
{
	m_hMemDC = NULL;
	m_hBitmap = NULL;
	m_hOldBitmap = NULL;
	m_cx = 0;
	m_cy = 0;
}

CBitmapMemDC::~CBitmapMemDC()
{
	_Clear();
}

void CBitmapMemDC::_Clear()
{
	if (NULL != m_hMemDC)
	{
		if (NULL != m_hOldBitmap)
		{
			SelectObject(m_hMemDC, m_hOldBitmap);
			m_hOldBitmap = NULL;
		}

		DeleteDC(m_hMemDC);		
		m_hMemDC = NULL;
	}
	if (NULL != m_hBitmap)
	{
		DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
	}	
}

BOOL CBitmapMemDC::IsDCCreated() const
{
	if (NULL != m_hMemDC && NULL != m_hBitmap)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CBitmapMemDC::CreateBitmapFromDC(int x, int y, int cx, int cy, HDC hDC, BOOL bBitblt/* = TRUE*/)
{
	_Clear();
	m_cx = (-1 == cx) ? GetSystemMetrics(SM_CXVIRTUALSCREEN) : cx;
	m_cy = (-1 == cy) ? GetSystemMetrics(SM_CYVIRTUALSCREEN) : cy;
	HDC hDCtmp = hDC;
	BOOL bGetDC = FALSE;
	if (NULL == hDCtmp)
	{
		hDCtmp = GetDC(::GetDesktopWindow());
		bGetDC = TRUE;
	}
	if (NULL == hDCtmp)
	{
		return FALSE;		
	}

	BOOL bOK = FALSE;	
	do 
	{
		m_hMemDC = CreateCompatibleDC(hDCtmp);
		if (NULL == m_hMemDC)
		{
			break;
		}
		m_hBitmap = CreateCompatibleBitmap(hDCtmp, m_cx, m_cy);
		if (NULL == m_hBitmap)
		{
			break;
		}
		m_hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hBitmap);
		
		if (bBitblt && !BitBlt(m_hMemDC, 0, 0, m_cx, m_cy, hDCtmp, x, y, SRCCOPY | CAPTUREBLT))
		{
			break;
		}

		bOK = TRUE;
	} while (FALSE);
	
	if (bGetDC)
	{
		ReleaseDC(::GetDesktopWindow(), hDCtmp);
	}

	if (!bOK)
	{
		_Clear();
	}

	return bOK;
}

BOOL CBitmapMemDC::AddColorMaskToBitmap(COLORREF clr, BYTE byTransparent)
{
	if (!IsDCCreated())
	{
		return FALSE;
	}

	BOOL bOK = FALSE;
	HDC hMemDC = NULL;
	HBITMAP hBitmap = NULL;
	
	do 
	{
		hMemDC = CreateCompatibleDC(m_hMemDC);
		if (NULL == hMemDC)
		{
			break;
		}
		hBitmap = CreateCompatibleBitmap(m_hMemDC, m_cx, m_cy);
		if (NULL == hBitmap)
		{
			break;
		}
		HBITMAP hBitmapOld = (HBITMAP)SelectObject(hMemDC, hBitmap);
		HBRUSH hbrush = CreateSolidBrush(clr);		
		RECT rc = {0};
		rc.right = m_cx;
		rc.bottom = m_cy;
		if (hbrush)
		{
			FillRect(hMemDC, &rc, hbrush);
			DeleteObject(hbrush);
		}
		
		BLENDFUNCTION   blendFunction;   
		blendFunction.BlendFlags=0;   
		blendFunction.BlendOp=AC_SRC_OVER;   
		blendFunction.SourceConstantAlpha = byTransparent;   
		blendFunction.AlphaFormat=0;			  			
		AlphaBlend(m_hMemDC, 0, 0, m_cx, m_cy, hMemDC, 0, 0, m_cx, m_cy, blendFunction);
		SelectObject(hMemDC, hBitmapOld);		
		
		bOK = TRUE;
	} while (FALSE);

	if (NULL != hBitmap)
	{
		DeleteObject(hBitmap);
		hBitmap = NULL;
	}
	if (NULL != hMemDC)
	{
		DeleteDC(hMemDC);
		hMemDC = NULL;
	}

	return bOK;
}
