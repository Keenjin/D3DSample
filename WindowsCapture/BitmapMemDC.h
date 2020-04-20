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

	//���忽������͸�ֵ����
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
	*	@brief ��ָ��DC��ָ���������򴴽�һ��λͼ
	*	@param	[in] (x, y) ���ϵ�����
	*	@param	[in] cx	��ȣ�-1ʱʹ������������Ļ�Ŀ��
	*	@param	[in] cy �߶ȣ�-1ʱʹ������������Ļ�ĸ߶�
	*	@param	[in] hDC ���Ϊ�գ�ʹ������DC
	*/
	BOOL CreateBitmapFromDC(int x, int y, int cx, int cy, HDC hDC, BOOL bBitblt = TRUE);

	/**
	*	@brief ��λͼ������ɫ
	*	@param	[in] clr Ҫ���ӵ���ɫ
	*	@param	[in] byTransparent ͸����
	*/
	BOOL AddColorMaskToBitmap(COLORREF clr, BYTE byTransparent);
	
	BOOL IsDCCreated() const;	///<����Ҫ����ֵ�����еĳ������õ��ã����Ҳ����ɳ���		
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
