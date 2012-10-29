/*
 * xara-cairo, a vector drawing program
 *
 * Based on Xara XL, Copyright (C) 1993-2006 Xara Group Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

// Defines the NodeGuideline class

#ifndef INC_GUIDES_H
#define INC_GUIDES_H

#include "snap.h"
//#include "dialogop.h"
#if !defined(EXCLUDE_FROM_RALPH)
#include "optsgrid.h"
#endif

class RenderRegion;
class Spread;
class ContextMenu;
class KeyPress;

/***********************************************************************************************

>	 class NodeGuideline : public NodeRenderableInk

	 Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	 Created:		5/9/95
     Base Classes: 	NodeRenderableInk
	 Purpose:		Defines the NodeGuideline Class

	 SeeAlso:		NodeRenderableInk
	
***********************************************************************************************/

class NodeGuideline : public NodeRenderableInk
{   
	CC_DECLARE_DYNAMIC( NodeGuideline ) 

public:  
	NodeGuideline(); 

	virtual String Describe(BOOL Plural, BOOL Verbose);    		   	
		   		   
	virtual SubtreeRenderState RenderSubtree(RenderRegion* pRender, Node** ppNextNode = NULL, BOOL bClip = TRUE);
	virtual void Render( RenderRegion* pRender );
	virtual DocRect	GetBoundingRect(BOOL DontUseAttrs=FALSE, BOOL HitTest=FALSE);
	virtual BOOL OnClick( DocCoord PointerPos, ClickType Click, ClickModifiers ClickMods, Spread *pSpread );
	virtual BOOL OnNodePopUp(Spread* pSpread, DocCoord PointerPos, ContextMenu* pMenu);
	virtual void SetSelected(BOOL Status);
	virtual void Transform( TransformBase& Trans );

	virtual void PreExportRender(RenderRegion* pRegion);
	virtual BOOL ExportRender(RenderRegion* pRegion);
	virtual BOOL NeedsToExport(RenderRegion *pRender,BOOL VisibleLayersOnly = FALSE, BOOL CheckSelected = FALSE);

	static DocRect GetRenderRect(MILLIPOINT Ordinate,GuidelineType Type,BOOL HitTest= FALSE, RenderRegion* pRender = NULL);
	DocRect GetRenderRect(BOOL HitTest = FALSE, RenderRegion* pRender = NULL);
		
	#ifdef _DEBUG						  
	virtual void ShowDebugTreeDetails() const;   
	#endif
	
	virtual void GetDebugDetails(StringBase* Str);   // This is used by the Debug Tree dialog
													 // It will probably be deleted when we ship !. 
	virtual BOOL Snap(DocCoord* pDocCoord);
	virtual BOOL Snap(DocRect* pDocRect,const DocCoord& PrevCoord,const DocCoord& CurCoord);

	void SetType(GuidelineType NewType)  { Type = NewType; }
	GuidelineType GetType() { return Type; }

	void MakeHorzGuideline() { Type = GUIDELINE_HORZ; }
	void MakeVertGuideline() { Type = GUIDELINE_VERT; }

	// Set & Get funcs that take Spread coords
	void 	 	SetOrdinate(MILLIPOINT NewOrdinate) { Ordinate = NewOrdinate; }
	MILLIPOINT 	GetOrdinate() { return Ordinate; }

	// Functions for converting from Spread to UserCoords, and back again.
	static MILLIPOINT ToSpreadOrdinate(Spread* pSpread,MILLIPOINT UserOrdinate,GuidelineType Type);
	static MILLIPOINT ToUserOrdinate(Spread* pSpread,MILLIPOINT SpreadOrdinate,GuidelineType Type);

	virtual void PolyCopyNodeContents(NodeRenderable* pNodeCopy);

	// Version 2 file format functions
	virtual BOOL WritePreChildrenWeb(BaseCamelotFilter* pFilter);
	virtual BOOL WritePreChildrenNative(BaseCamelotFilter* pFilter);

private:
	virtual Node* SimpleCopy();   
	void CopyNodeContents(NodeGuideline* NodeCopy);

	MILLIPOINT 	ExtractOrdinate(DocCoord* pDocCoord);
	void 		ReplaceOrdinate(DocCoord* pDocCoord,MILLIPOINT Ordinate);
	void 		TranslateRect(DocRect* pDocRect,MILLIPOINT Delta);

	static MILLIPOINT GetScaledPixelWidth(RenderRegion* pRender = NULL);

	// REMEMBER TO UPDATE CopyNodeContents

	GuidelineType	Type;
	MILLIPOINT		Ordinate;
};

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Types of op the OpGuideline class can do

enum OpGuidelineOpMethod  {	GUIDELINEOPMETHOD_INVALID,				// used to invalidate static data
							GUIDELINEOPMETHOD_MOVE_DRAG,			// Move the guideline via a drag
							GUIDELINEOPMETHOD_NEW_DRAG,				// Create a new guideline via a drag
							GUIDELINEOPMETHOD_MOVE_IMMEDIATE,		// Move the guideline immediately
							GUIDELINEOPMETHOD_NEW_IMMEDIATE,		// Create a new guideline immediately
							GUIDELINEOPMETHOD_DELETE,				// Deletes guidelines
							GUIDELINEOPMETHOD_CREATE_GUIDE_LAYER	// Create guide layer
						  };

// WEBSTER - markn 15/1/97
// Remove the same bits Ralph removes from the guides system
#ifndef WEBSTER
#if !defined(EXCLUDE_FROM_RALPH)

/********************************************************************************************

>	class OpGuidelineParam : public OpParam

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	2/10/95
	Purpose:	Param passed to OpGuideline::DoWithParams()

********************************************************************************************/

class OpGuidelineParam : public OpParam
{
	CC_DECLARE_MEMDUMP(OpGuidelineParam)

public:
	OpGuidelineOpMethod Method;

	// for type == GUIDELINEOPMETHOD_NEW
	GuidelineType	Type;					// The type of guideline to produce at the end of a drag

	// for type == GUIDELINEOPMETHOD_MOVE
	NodeGuideline*	pGuideline;				// The guideline to translate
	MILLIPOINT		NewOrdinate;			// The new absolute ordinate for the guideline

	// for type == GUIDELINEOPMETHOD_DELETE
	NodeGuideline** pGuidelineList;
};

/********************************************************************************************

>	class OpGuideline : public UndoableOperation

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	6/9/95
	Purpose:	Allows guidelines to be dragged around

********************************************************************************************/

class OpGuideline : public UndoableOperation
{

CC_DECLARE_DYNCREATE(OpGuideline);

public:
	#define OPTOKEN_GUIDELINE _T("Guideline")	

	// Construction/Destruction
	OpGuideline();
	~OpGuideline();

	virtual void DoWithParam(OpDescriptor* pOpDesc,OpParam* pParam);

	// Virtual functions needed for the dragging operations to work
	virtual BOOL DragKeyPress(KeyPress* pKeyPress, BOOL bSolidDrag);
	virtual void DragPointerMove( DocCoord PointerPos, ClickModifiers ClickMods, Spread*, BOOL bSolidDrag);
	virtual void DragFinished(	DocCoord PointerPos, 
								ClickModifiers ClickMods, Spread*, 
								BOOL Success, BOOL bSolidDrag);

	virtual BOOL Undo();
	virtual BOOL Redo();

	// Some Render functions to will draw the EORed drag boxes
	void RenderMyDragBlobs();
	void RenderDragBlobs(DocRect Rect,Spread* pSpread, BOOL bSolidDrag);
		
	// These functions required for the OpDescriptor class
	static BOOL Init();
	static OpState GetState(String_256* Description, OpDescriptor*);
	void GetOpName(String_256* OpName);

// WEBSTER - markn 15/1/97
// Moved static function CreateGuideLayer() into the Layer class, where it should be
//	static Layer* CreateGuideLayer();
	static BOOL IsMouseOverRuler();

protected:
	// The all important Do functions
	void DoDrag(Spread* pSpread,DocCoord PointerPos);

	void DoAddNewGuideline(Spread* pSpread,DocCoord PointerPos,GuidelineType Type);
	void DoMoveGuideline(Spread* pSpread,DocCoord PointerPos,NodeGuideline* pGuideline);

	BOOL DoTranslateGuideline(NodeGuideline* pGuideline,MILLIPOINT Ordinate,NodeGuideline** ppNewGuideline = NULL);
	BOOL DoNewGuideline(Node* pContext,AttachNodeDirection AttachDir,GuidelineType Type,MILLIPOINT Ordinate,NodeGuideline** ppNewGuideline = NULL);
	BOOL DoDeleteGuideline(NodeGuideline* pGuideline,BOOL TryToLeaveCopy = FALSE);
	BOOL DoDeleteListOfGuidelines(NodeGuideline** pGuidelineList);
	Layer* DoCreateGuideLayer();

	BOOL CanLeaveCopy();

	void UpdateStatusLineAndPointer();

	void BroadcastGuidelineChanges(NodeGuideline* pGuideline);
	void BroadcastGuidelineChanges(Layer* pLayer);

	void SetOrdinate(DocCoord& PointerPos);

private:
	OpGuidelineOpMethod OpMethod;

	// Stuff for dragging guidelines around
	Spread*        	pSpread;
	NodeGuideline* 	pDraggedGuideline;
	MILLIPOINT 	   	Ordinate;
	GuidelineType  	Type;
	BOOL		   	LeaveCopy;
	Cursor*			pCursor;
	UINT32			CurrentStatusHelp;
	INT32			CursorStackID;

	// Layer used by broadcast
	Layer* pBroadcastLayer;

	BOOL RenderOn;

protected:
	// The ID of the op used in the undo & redo menu items
	UINT32 UndoIDS;
};


/********************************************************************************************

>	class OpDeleteGuideline : public OpGuideline

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/10/95
	Purpose:	Allows guides to be Deleted

********************************************************************************************/

class OpDeleteGuideline : public OpGuideline
{

CC_DECLARE_DYNCREATE(OpDeleteGuideline);

public:
	#define OPTOKEN_DELETEGUIDELINE _T("DeleteGuideline")
	
	// Construction/Destruction
	OpDeleteGuideline();
	~OpDeleteGuideline();

	virtual void Do(OpDescriptor* pOpDesc);

	// These functions required for the OpDescriptor class
	static OpState GetState(String_256* Description, OpDescriptor*);

	static void SetGuideline(NodeGuideline* pThisGuideline) { pGuideline = pThisGuideline; }

protected:
	static NodeGuideline* pGuideline;
};

/********************************************************************************************

>	class OpDeleteAllGuidelines : public OpGuideline

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	5/10/95
	Purpose:	Allows guides to be Deleted

********************************************************************************************/

class OpDeleteAllGuidelines : public OpGuideline
{

CC_DECLARE_DYNCREATE(OpDeleteAllGuidelines);

public:
	#define OPTOKEN_DELETEALLGUIDELINES _T("DeleteAllGuidelines")
	
	// Construction/Destruction
	OpDeleteAllGuidelines();
	~OpDeleteAllGuidelines();

	virtual void Do(OpDescriptor* pOpDesc);

	// These functions required for the OpDescriptor class
	static OpState GetState(String_256* Description, OpDescriptor*);
};


/******************************************************************************
>	class OpNewGuideline : public OpGuideline

	Author:		Ed_Cornes (Xara Group Ltd) <camelotdev@xara.com>
	Created:	9/10/95
	Purpose:	op to create a new guideline immendiately
*******************************************************************************/

class OpNewGuideline : public OpGuideline
{
	CC_DECLARE_DYNCREATE(OpNewGuideline);

public:
	#define OPTOKEN_NEWGUIDELINE2 _T("NewGuideline")
	
	OpNewGuideline();
	~OpNewGuideline();
	static BOOL Init();

	virtual void Do(OpDescriptor* pOpDesc);

	static OpState GetState(String_256* pReasonGreyed, OpDescriptor* pOpDesc);

	static void SetNewGuidelineParam(GuidelineType Type, MILLIPOINT pos);

protected:
	static OpGuidelineParam NewGuidelineParam;
};


/********************************************************************************************
>	class OpSpreadOrigin : public OpGridResize

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	6/10/95
	Purpose:	Allows you to drag the origin to another place
********************************************************************************************/

class OpSpreadOrigin : public OpGridResize
{
	CC_DECLARE_DYNCREATE(OpSpreadOrigin);
	#define OPTOKEN_SPREADORIGIN _T("SpreadOrigin")	

public:
	OpSpreadOrigin();
	~OpSpreadOrigin();

	virtual void Do(OpDescriptor* pOpDesc);

	// Virtual functions needed for the dragging operations to work
	virtual void DragPointerMove( DocCoord PointerPos, ClickModifiers ClickMods, Spread*, BOOL bSolidDrag);
	virtual void DragFinished(	DocCoord PointerPos, 
								ClickModifiers ClickMods, Spread*, 
								BOOL Success, BOOL bSolidDrag);

	// Some Render functions to will draw the EORed drag boxes
	void RenderMyDragBlobs();
	void RenderDragBlobs(DocRect Rect,Spread* pSpread, BOOL bSolidDrag);
		
	// These functions required for the OpDescriptor class
	static BOOL Init();
	static OpState GetState(String_256* Description, OpDescriptor*);

protected:
	// The all important Do functions
	void DoDrag(Spread* pSpread,DocCoord PointerPos);
	BOOL DoChangeOrigin(Spread* pSpread,DocCoord NewOrigin);

private:
	// Stuff for dragging guidelines around
	Spread*  pSpread;
	DocCoord CurrentOrigin;
	BOOL RenderOn;
};


/******************************************************************************
>	class OpResetSpreadOrigin : public OpSpreadOrigin

	Author:		Ed_Cornes (Xara Group Ltd) <camelotdev@xara.com>
	Created:	10/10/95
	Purpose:	op to reset the spread (user and grid) origin
*******************************************************************************/

class OpResetSpreadOrigin : public OpSpreadOrigin
{
	CC_DECLARE_DYNCREATE(OpResetSpreadOrigin);
	#define OPTOKEN_RESETSPREADORIGIN _T("ResetSpreadOrigin")
	
public:
	OpResetSpreadOrigin();
	static BOOL Init();

	virtual void Do(OpDescriptor* pOpDesc);
	static OpState GetState(String_256* pReasonGreyed, OpDescriptor* pOpDesc);
};


/********************************************************************************************
>	class GuidelinePropDlgParams

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	29/9/95
	Purpose:	Parameters needed for the guideline properties dlg to function correctly
********************************************************************************************/

class GuidelinePropDlgParams
{
public:
	GuidelinePropDlgParams() { pGuideline = NULL; }

	NodeGuideline*	pGuideline;		// Can be NULL

	// If pGuideline is NULL the following params are used to create a new Guideline
	MILLIPOINT 		Ordinate;		// Ordinate in Spread coords
	GuidelineType	Type;			// The type of new Guideline
};


/********************************************************************************************
>	class GuidelinePropDlg: public DialogOp

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	29/9/95
	Purpose:	Allows the user to change the properties of a guideline
********************************************************************************************/

class GuidelinePropDlg: public DialogOp
{         
	CC_DECLARE_DYNCREATE( GuidelinePropDlg )  
	#define OPTOKEN_EDITGUIDELINEPROPDLG _T("EditGuidelinePropDlg")
	#define OPTOKEN_NEWGUIDELINEPROPDLG   _T("NewGuidelinePropDlg")

public:
	GuidelinePropDlg();
	void Do(OpDescriptor*);

	static const INT32 IDD;
	static const CDlgMode Mode;

	static OpState GetState(String_256*, OpDescriptor*);	
	static BOOL Init();

	MsgResult Message(Msg* Message);

	void ShowDetails();
	BOOL CommitValues();

	static void SetNewGuidelineParams(GuidelineType type, MILLIPOINT pos);
	static void SetEditGuidelineParams(NodeGuideline* pGuide);

private:
	static GuidelinePropDlgParams Params;
}; 

#endif
#endif // WEBSTER
#endif // INC_GUIDES_H
