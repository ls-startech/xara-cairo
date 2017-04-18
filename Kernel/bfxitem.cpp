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


#include "camtypes.h"

#include "bfxitem.h"

//#include "plugres.h"	// _R(IDS_PLUGIN_)....
#include "wbitmap.h"	// WinBitmap
#include "bmpcomp.h"	// BitmapList
//#include "fixmem.h"		// CCMalloc, CCFree
//#include "progress.h"	// Progress

#include "bfxdlg2.h"	// BfxPlugInDlg

// Place any IMPLEMENT type statements here
CC_IMPLEMENT_DYNAMIC(BfxPlugIn, PlugInItem)
CC_IMPLEMENT_DYNAMIC(BfxSpecialEffects, BfxPlugIn)
CC_IMPLEMENT_DYNAMIC(BfxBrightnessContrast, BfxPlugIn)
CC_IMPLEMENT_DYNAMIC(BfxResize, BfxPlugIn)
CC_IMPLEMENT_DYNAMIC(BfxFlipAndRotate, BfxPlugIn)
CC_IMPLEMENT_DYNAMIC(BfxColourDepth, BfxPlugIn)

// We want better memory tracking
#define new CAM_DEBUG_NEW

/**************************************************************************************

>	BfxPlugIn::BfxPlugIn()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Purpose:	Constructor for BfxPlugIn. 

**************************************************************************************/

BfxPlugIn::BfxPlugIn()
{
}

/**************************************************************************************

>	BfxPlugIn::~BfxPlugIn()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Purpose:	Destructor for BfxPlugIn. 

**************************************************************************************/

BfxPlugIn::~BfxPlugIn()
{
}

/**************************************************************************************

>	virtual String_32 BfxPlugIn::GetUniqueID() const

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Returns:	A string containing a filetype. 
	Purpose:	A string which defines the unique identifier for the plug-ins that this
				handler is interested in.
				In this case being Bfx it is 'BFX:'.

**************************************************************************************/

String_32 BfxPlugIn::GetUniqueID() const
{
	return TEXT("BFX:");
}

/**************************************************************************************

>	virtual TypeOfPlugIn BfxPlugIn::GetTypeOfPlugIn() const

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Returns:	Type of the plug-in. 
	Purpose:	A unique type for the plug-ins that this handler is interested in.
				In this case being Bfx it is PLUGIN_BFX.

**************************************************************************************/

TypeOfPlugIn BfxPlugIn::GetTypeOfPlugIn() const
{
	return PLUGIN_BFX;
}

/**************************************************************************************

>	virtual BOOL BfxPlugIn::Apply(KernelBitmap *pInputBitmap, Document* pDocument,
								  KernelBitmap **ppOutputBitmap = NULL)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Inputs:		pInputBitmap	the bitmap to apply the effect to
				pDocument		the document this bitmap is in
	Outputs:	ppOutputBitmap	if specified, a pointer to the output bitmap generated
	Returns:	True if works ok, False otherwise.
	Purpose:	Applies a plug-in's bitmap effect to the specified bitmap. 

**************************************************************************************/

BOOL BfxPlugIn::Apply(KernelBitmap *pInputBitmap, Document* pDocument,
					  KernelBitmap **ppOutputBitmap)
{
	ERROR2IF(pInputBitmap == NULL, FALSE, "BfxPlugIn::Apply null bitmap specified!");
	ERROR2IF(pDocument == NULL, FALSE, "BfxPlugIn::Apply null document specified!");

	KernelBitmap * pBmp = NULL;
	BOOL ok = ApplyBitmapEffect(pInputBitmap, pDocument, &pBmp);

	if (ppOutputBitmap)
		*ppOutputBitmap = pBmp;
	
	return ok;
}

/**************************************************************************************

>	virtual BOOL BfxPlugIn::IsBitmapModeOk(INT32 ColourDepth)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Inputs:		ColourDepth		the colour depth or bits per pixel of the bitmap
				GreyScale		only relevent if bitmap is 8bpp, True if greyscale
	Returns:	True if the bitmap is compatible and False otherwise.
	Purpose:	Checks to see if a plug-in can cope with the specified colour depth.

**************************************************************************************/

BOOL BfxPlugIn::IsBitmapModeOk(INT32 ColourDepth, BOOL GreyScale)
{
	// Passed the checks ok so return TRUE
	return TRUE;
}


/**************************************************************************************

>	BOOL BfxPlugIn::ApplyBitmapEffect(KernelBitmap *pInputBitmap, Document* pDocument,
									  KernelBitmap **ppOutputBitmap = NULL)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Inputs:		pInputBitmap	the bitmap to apply the effect to
				pDocument		the document this bitmap is in
	Outputs:	ppOutputBitmap	if specified, a pointer to the output bitmap generated
	Returns:	True if works ok, False otherwise.
	Purpose:	Applies a plug-in's bitmap effect to the specified bitmap.

**************************************************************************************/

BOOL BfxPlugIn::ApplyBitmapEffect(KernelBitmap *pInputBitmap, Document* pDocument,
								  KernelBitmap **ppOutputBitmap)
{
	ERROR2IF(pInputBitmap == NULL, FALSE, "BfxPlugIn::Apply null bitmap specified!");

	if (!pInputBitmap->DestroyXPEInfo())
		return FALSE;

	OpParam Param;
	Param.Param1 = (INT32)pInputBitmap;
	Param.Param2 = (INT32)pDocument;
	// give it a variable to return the result i.e. ok/cancel in
	BOOL ok = FALSE;
	Param.Output = &ok;
	// Obtain a pointer to the op descriptor for the create operation 
	OpDescriptor* pOpDesc = GetOpDescriptor();
	ERROR2IF(pOpDesc==NULL,FALSE,"Failed to find the BfxPlugInDlg OpDescriptor");

	// Only one live instance of the operation is allowed. It's probably a dialog 
	if (!MessageHandler::MessageHandlerExists(CC_RUNTIME_CLASS(BfxPlugInDlg)))
	{
		pOpDesc->Invoke(&Param);		 

		// **** Warning ****
		// This assumes that the dialog box is modal and so the OpParam is still around
		// rather than having been deleted 
		// return the output bitmap to the caller
		if (ppOutputBitmap)
			*ppOutputBitmap = (KernelBitmap*)Param.Param1;
		// return the result i.e. ok/cancel to the caller
		return ok;
	}
/*	else
	{
		BROADCAST_TO_CLASS(BfxMsg(pBitmap),DialogOp);	
	}	*/

	// Everything seemed to work ok
	return TRUE;
}

/**************************************************************************************

>	virtual BOOL BfxPlugIn::Apply(WinBitmap *pInputDIB, WinBitmap * pOutputDIB) 

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/3/97
	Inputs:		pInputDIB		the source bitmap
	Outputs:	pOutputDib	the destination bitmap
	Returns:	True if works ok, False otherwise.
	Purpose:	Ask a plug-in to strut its stuff and do its work taking the pDib as a
				source and pOutputDIB as the result.
				If ApplyLast is True then asks a plug-in to strut its stuff again
				using the same parameters as last time. Once again doing its work
				taking the pDib as a source and	pOutputDIB as the result.
				Assumes pDib and pOutputDib already allocated and are the same size etc.

**************************************************************************************/

/* BOOL BfxPlugIn::Apply(WinBitmap *pInputDIB, WinBitmap * pOutputDIB) 
{
	ERROR2IF(pInputDIB == NULL || pOutputDIB == NULL,FALSE,"Input bitmaps not specified correctly");


	return ok;
} */

/**************************************************************************************
/// The different types of BFX plug-ins available
**************************************************************************************/

/**************************************************************************************
/// BfxBrightnessContrast
**************************************************************************************/

/**************************************************************************************

>	BfxBrightnessContrast::BfxBrightnessContrast()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Purpose:	Constructor for a BfxBrightnessContrast plug-in. 

**************************************************************************************/

BfxBrightnessContrast::BfxBrightnessContrast()
{
	SetFamilyName(_R(IDS_BFXDLG));
	SetPlugInName(_R(IDS_BFXPLUGINDLG_BRIGHTNESS));

	//SetFilterName(TEXT("Fred"));
	//SetModuleName(TEXT("Jim"));
}

/**************************************************************************************

>	virtual OpDescriptor * BfxBrightnessContrast::GetOpDescriptor()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Returns:	the OpDescriptor for the connected dialog box operation
	Purpose:	Returns the OpDescriptor for the dialog box connected with this bfx
				plug-in ready to be invoked.

**************************************************************************************/

OpDescriptor * BfxBrightnessContrast::GetOpDescriptor()
{
	return OpDescriptor::FindOpDescriptor(CC_RUNTIME_CLASS(BfxBrightnessContrastDlg));
}

/**************************************************************************************
/// BfxSpecialEffects
**************************************************************************************/

/**************************************************************************************

>	BfxSpecialEffects::BfxSpecialEffects()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Purpose:	Constructor for a BfxSpecialEffects plug-in. 

**************************************************************************************/

BfxSpecialEffects::BfxSpecialEffects()
{
	SetFamilyName(_R(IDS_BFXDLG));
	SetPlugInName(_R(IDS_BFXPLUGINDLG_SPECIAL));
}

/**************************************************************************************

>	virtual OpDescriptor * BfxSpecialEffects::GetOpDescriptor()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Returns:	the OpDescriptor for the connected dialog box operation
	Purpose:	Returns the OpDescriptor for the dialog box connected with this bfx
				plug-in ready to be invoked.

**************************************************************************************/

OpDescriptor * BfxSpecialEffects::GetOpDescriptor()
{
	return OpDescriptor::FindOpDescriptor(CC_RUNTIME_CLASS(BfxSpecialEffectsDlg));
}

/**************************************************************************************
/// BfxResize
**************************************************************************************/

/**************************************************************************************

>	BfxResize::BfxResize()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Purpose:	Constructor for a BfxResize plug-in. 

**************************************************************************************/

BfxResize::BfxResize()
{
	SetFamilyName(_R(IDS_BFXDLG));
	SetPlugInName(_R(IDS_BFXPLUGINDLG_RESIZE));
}

/**************************************************************************************

>	virtual OpDescriptor * BfxResize::GetOpDescriptor()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Returns:	the OpDescriptor for the connected dialog box operation
	Purpose:	Returns the OpDescriptor for the dialog box connected with this bfx
				plug-in ready to be invoked.

**************************************************************************************/

OpDescriptor * BfxResize::GetOpDescriptor()
{
	return OpDescriptor::FindOpDescriptor(CC_RUNTIME_CLASS(BfxResizeDlg));
}

/**************************************************************************************
/// BfxFlipAndRotate
**************************************************************************************/

/**************************************************************************************

>	BfxFlipAndRotate::BfxFlipAndRotate()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Purpose:	Constructor for a BfxFlipAndRotate plug-in. 

**************************************************************************************/

BfxFlipAndRotate::BfxFlipAndRotate()
{
	SetFamilyName(_R(IDS_BFXDLG));
	SetPlugInName(_R(IDS_BFXPLUGINDLG_FLIP));
}

/**************************************************************************************

>	virtual OpDescriptor * BfxFlipAndRotate::GetOpDescriptor()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Returns:	the OpDescriptor for the connected dialog box operation
	Purpose:	Returns the OpDescriptor for the dialog box connected with this bfx
				plug-in ready to be invoked.

**************************************************************************************/

OpDescriptor * BfxFlipAndRotate::GetOpDescriptor()
{
	return OpDescriptor::FindOpDescriptor(CC_RUNTIME_CLASS(BfxFlipAndRotateDlg));
}

/**************************************************************************************
/// BfxColourDepth
**************************************************************************************/

/**************************************************************************************

>	BfxColourDepth::BfxColourDepth()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Purpose:	Constructor for a BfxColourDepth plug-in. 

**************************************************************************************/

BfxColourDepth::BfxColourDepth()
{
	SetFamilyName(_R(IDS_BFXDLG));
	SetPlugInName(_R(IDS_BFXPLUGINDLG_COLOUR));
}

/**************************************************************************************

>	virtual OpDescriptor * BfxColourDepth::GetOpDescriptor()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/3/97
	Returns:	the OpDescriptor for the connected dialog box operation
	Purpose:	Returns the OpDescriptor for the dialog box connected with this bfx
				plug-in ready to be invoked.

**************************************************************************************/

OpDescriptor * BfxColourDepth::GetOpDescriptor()
{
	return OpDescriptor::FindOpDescriptor(CC_RUNTIME_CLASS(BfxColourDepthDlg));
}
