// BitmapMemDC.h: interface for the CBitmapMemDC class.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>

#pragma comment( lib, "Msimg32.lib" )

#pragma once
class CBitmapMemDC  
{
public:	
	CBitmapMemDC();
	virtual ~CBitmapMemDC();

	//定义拷贝构造和赋值构造
	CBitmapMemDC(const CBitmapMemDC& other)
	{
		if (this != &other && other.IsDCCreated())
		{
			CreateBitmapFromDC(0, 0, other.GetWidth(), other.GetHeight(), other);
		}
	}
	const CBitmapMemDC& operator =(const CBitmapMemDC& other)
	{
		if (this != &other && other.IsDCCreated())
		{
			_Clear();
			CreateBitmapFromDC(0, 0, other.GetWidth(), other.GetHeight(), other);
		}
		
		return *this;
	}

	/**
	*	@brief 从指定DC的指定矩形区域创建一个位图
	*	@param	[in] (x, y) 左上的坐标
	*	@param	[in] cx	宽度，-1时使用整个虚拟屏幕的宽度
	*	@param	[in] cy 高度，-1时使用整个虚拟屏幕的高度
	*	@param	[in] hDC 如果为空，使用桌面DC
	*/
	BOOL CreateBitmapFromDC(int x, int y, int cx, int cy, HDC hDC, BOOL bBitblt = TRUE);

	/**
	*	@brief 给位图叠加颜色
	*	@param	[in] clr 要叠加的颜色
	*	@param	[in] byTransparent 透明度
	*/
	BOOL AddColorMaskToBitmap(COLORREF clr, BYTE byTransparent);
	
	BOOL IsDCCreated() const;	///<由于要被赋值构造中的常量引用调用，因此也定义成常量		
	operator HDC() const{return m_hMemDC;}
	operator HBITMAP() const{return m_hBitmap;}
	int GetWidth() const{return m_cx;}
	int GetHeight() const{return m_cy;}

protected:
	void _Clear();
	

protected:
	HDC		m_hMemDC;
	HBITMAP	m_hBitmap;
	HBITMAP	m_hOldBitmap;
	int m_cx;
	int m_cy;

};
