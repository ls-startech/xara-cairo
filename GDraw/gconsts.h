/*
 * xara-cairo, a vector drawing program
 *
 * Based on Xara LX, Copyright (C) 1993-2006 Xara Group Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2.
 *
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */


#ifndef INC_GCONSTS
#define	INC_GCONSTS

#if !defined(__WXMSW__)
const BYTE PT_CLOSEFIGURE	= 1;
const BYTE PT_LINETO		= 2;
const BYTE PT_BEZIERTO		= 4;
const BYTE PT_MOVETO		= 6;
const BYTE PT_PATHELEMENT	= 6;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// These are the error codes returned by GDraw.dll. Use GetLastError to obtain the error code.
//
enum eError {
	GERROR_NO_ERROR,
	GERROR_BAD_BITMAP,
	GERROR_BAD_GRADUATION_STYLE,
	GERROR_BAD_GRADUATION_TABLE,
	GERROR_BAD_TILE_STYLE,
	GERROR_BAD_TRANSPARENCY_STYLE,
	GERROR_BAD_PARAMETERS,
	GERROR_BAD_PATH,
	GERROR_CORRUPT_EDGE_TABLE,
	GERROR_INCOMPATIBLE_BITMAPS,
	GERROR_INCOMPATIBLE_REGION,
	GERROR_INVALID_SCANLINE_ROUTINE,
	GERROR_NO_BITMAP,
	GERROR_NOT_32_BITS_PER_PIXEL,
	GERROR_OUT_OF_MEMORY,
	GERROR_OUTPUT_PATH_TOO_COMPLEX,
	GERROR_STACK_OVERFLOW,
	GERROR_UNDEFINED_FILL_STYLE,
	GERROR_UNDEFINED_MEMORY_HANDLERS,
	GERROR_UNIMPLEMENTED,
	GERROR_FAILED
} ;

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// The following is for use by my test program only.
//
enum ColourEnum {
	C_SOLID,
	C_LINEAR,
	C_RADIAL,
	C_CONICAL,
	C_SQUARE,
	C_MULTIRADIAL,
	C_3COLOUR,
	C_4COLOUR,
	C_GOURAND,
	C_TILE,
	C_XOR,
	C_INVALID=99
} ;

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Cap and join styles - as used by GDraw_Stroke etc
//
enum CapStyles {
	CAPS_BUTT	= 0,
	CAPS_ROUND	= 1,
	CAPS_SQUARE	= 2
} ;

enum JoinStyles {
	JOIN_MITER	= 0,
	JOIN_ROUND	= 1,
	JOIN_BEVEL	= 2
} ;

//
// Bitmap formats - as used by GDraw_SetDIBitmap and GDraw_ConvertBitmap.
//
enum BitmapFormat {
	FORMAT8BPP_NORMAL = 0,
	FORMAT8BPP_GREY   = 1,
	FORMAT16BPP_565   = 0,
	FORMAT16BPP_655   = 1,
	FORMAT16BPP_555   = 2,
	FORMAT16BPP_664   = 3
} ;

//
// Transparency styles - as used by GColour_SetTransparency, GColour_SetGraduation etc
//
enum TransparencyEnum {
	T_NONE,
	T_REFLECTIVE,
	T_SUBTRACTIVE,
	T_ADDITIVE,
	T_FLAT_REFLECTIVE,
	T_FLAT_SUBTRACTIVE,
	T_FLAT_ADDITIVE,
	T_GRAD_REFLECTIVE,
	T_GRAD_SUBTRACTIVE,
	T_GRAD_ADDITIVE,
	T_SPECIAL_1,
	T_SPECIAL_2,
	T_SPECIAL_3,
	T_CONTRAST,		T_FLAT_CONTRAST,	T_GRAD_CONTRAST,
	T_SATURATION,	T_FLAT_SATURATION,	T_GRAD_SATURATION,
	T_DARKEN,		T_FLAT_DARKEN,		T_GRAD_DARKEN,
	T_LIGHTEN,		T_FLAT_LIGHTEN,		T_GRAD_LIGHTEN,
	T_BRIGHTNESS,	T_FLAT_BRIGHTNESS,	T_GRAD_BRIGHTNESS,
	T_LUMINOSITY,	T_FLAT_LUMINOSITY,	T_GRAD_LUMINOSITY,
	T_HUE,			T_FLAT_HUE,			T_GRAD_HUE,
	T_BEVEL,		T_FLAT_BEVEL,		T_GRAD_BEVEL
} ;
//
// Graduation styles - as used by GColour_SetGraduation
//
enum GradEnum {
	GRAD_LINEAR,
	GRAD_RADIAL,
	GRAD_CONICAL,
	GRAD_SQUARE,
	GRAD_3COLOUR,
	GRAD_4COLOUR
};

//
// Dither styles - used by GColour_SetDitherStyle and GColour_ConvertBitmap
//
enum DitherStyle {
	DITHER_DIFFUSED,
	DITHER_ORDERED,
	DITHER_GREY_ORDERED,
	DITHER_FLOYD_STEINBERG,
	DITHER_NONE,
	DITHER_SIMPLE_GREYSCALE,
	DITHER_DIFFUSED_GREYSCALE,
	DITHER_EXACT
} ;

/////////////////////////////////////////////////////////////////////////////////////////////////


struct GCONTEXT {
	DWORD Valid ;			// Valid if set to C90FDAA2.
	DWORD Data[1] ;
};


struct DitherBlock {
	DWORD Data[4] ;
} ;

struct GraduationTable {
	DWORD Length ;
	COLORREF StartColour ;
	COLORREF EndColour ;
	DitherBlock Table[0x100] ;
} ;

struct GraduationTable32 {				// This structure is used with 32bpp bitmaps.
	DWORD Length ;
	COLORREF StartColour ;
	COLORREF EndColour ;
	COLORREF Table[0x100] ;
} ;

struct TransparentGradTable {
	BYTE Table[0x100] ;
} ;


struct RGBT {
	BYTE Red ;
	BYTE Green ;
	BYTE Blue ;
	BYTE Transparency ;
} ;

struct BGRT {
	BYTE Blue ;
	BYTE Green ;
	BYTE Red ;
	BYTE Transparency ;
} ;

struct BGR {
	BYTE Blue ;
	BYTE Green ;
	BYTE Red ;
	BYTE Unused ;
} ;



struct DashType {
	DWORD Length ;
	DWORD Offset ;
	DWORD Array[8] ;
} ;


struct REGION {
	DWORD Type ;	// 0 = non-antialiased ; 1 = 5 line antialiased ; 2 = 9 line antialias
	RECT Rect ;
	DWORD Data[1] ;
} ;

#ifndef INC_CAMTYPES
	typedef __int64 XLONG ;
//	typedef struct {
//		UINT32 lo ;
//		 INT32 hi ;
//	} XLONG ;
#endif

struct GMATRIX {
	INT32 AX ;
	INT32 AY ;
	INT32 BX ;
	INT32 BY ;
	XLONG CX ;
	XLONG CY ;
} ;



struct STATISTICS {
	COLORREF C ;
	 INT32 N  ;
	XLONG R  ;
	XLONG R2 ;
	XLONG RX ;
	XLONG RY ;
	XLONG G  ;
	XLONG G2 ;
	XLONG GX ;
	XLONG GY ;
	XLONG B  ;
	XLONG B2 ;
	XLONG BX ;
	XLONG BY ;
	XLONG X  ;
	XLONG Y  ;
	XLONG X2 ;
	XLONG Y2 ;
	XLONG XY ;
} ;



//struct GBLOB {
//	POINT	PointA ;
//	POINT	PointB ;
//	POINT	PointC ;
//	BYTE	Scale ;
//} ;

/////////////////////////////////////////////////////////////////////////////////////////////////

const INT32 FX = 14 ;
const INT32 MAX_BITMAP_WIDTH  = 0x800 ;
const INT32 MAX_BITMAP_HEIGHT = 0x800 ;

/////////////////////////////////////////////////////////////////////////////////////////////////

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
