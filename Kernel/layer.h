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


// declares LayerStatus and Layer classes

#ifndef INC_LAYER
#define	INC_LAYER


class Document;
class DocView;
class DocColour;
class IndexedColour;
class LayerMsgHandler;
//class KernelBitmapRef;

/********************************************************************************************

>	class LayerStatus: public CCObject  

	Author:		Simon_Maneggio (Xara Group Ltd) <camelotdev@xara.com>
	Created:	4/1/94
	Purpose:	This class (structure) holds layer information which can be changed by the 
				user.  
				The reason that this information is held in a seperate class is that it needs 
				to be stored in both the Layer and Layer Manager classes. 
	SeeAlso:	Layer
				LayerManager

********************************************************************************************/


class LayerStatus  
{
public: 
	String_256 StringLayerID; // Layer identifier. This should be unique within a spread

	LayerStatus(); 
	LayerStatus(LayerStatus& ls); 
	INT32 operator==(LayerStatus); 
}; 

/***********************************************************************************************

>	class Layer : public NodeRenderablePaper

	Author:		Simon_Maneggio (Xara Group Ltd) <camelotdev@xara.com>
	Created:	25/4/93
	Purpose:	Layers optionally exist on a page, they group together objects which are drawn
				at a particular level in a document. 
				
	SeeAlso:	NodeRenderablePaper   
	SeeAlso:    Chapter
	SeeAlso:    Spread
	SeeAlso:    Page

***********************************************************************************************/

class Layer: public NodeRenderablePaper
{   
	CC_DECLARE_DYNAMIC( Layer ) 
public:

	Layer();   
	~Layer();
	
	Layer(Node* ContextNode,  
		  AttachNodeDirection Direction,
		  const String_256 &LayerID, 
		  BOOL Locked=FALSE, 
		  BOOL Mangled=FALSE,  
		  BOOL Marked=FALSE, 
		  BOOL Selected=FALSE 
		  );      
	
	static BOOL Init();
	static void Deinit();

    String Describe(BOOL Plural, BOOL Verbose);    	
 
	virtual SubtreeRenderState RenderSubtree(RenderRegion *pRender, Node** ppNextNode = NULL, BOOL bClip = TRUE);
	virtual void RenderAfterSubtree(RenderRegion *pRender);

	virtual BOOL NeedsToExport(RenderRegion *pRender,BOOL VisibleLayersOnly = FALSE,
							   BOOL CheckSelected = FALSE);
	void PreExportRender(RenderRegion* pRegion);
	BOOL ExportRender(RenderRegion* pRegion);
	BOOL IsLayer() const;

 	// obtains its page's pasteboard rectangle
 	// obtains its page's pasteboard rectangle
 	DocRect GetPasteboardRect(BOOL Pixelise = TRUE, View *pView = NULL) const;
 	  
 	#ifdef _DEBUG
 	void ShowDebugTreeDetails() const;
    #endif  

	Layer* FindNextLayer(BOOL ExcludeInvisible = FALSE, BOOL ExcludeLocked = FALSE);
	Layer* FindPrevLayer(BOOL ExcludeInvisible = FALSE, BOOL ExcludeLocked = FALSE);

	// Special frame related find layer functions
	Layer* FindNextFrameLayer(); 
	Layer* FindPrevFrameLayer(); 

	// Called when a node is being hidden
	virtual BOOL HidingNode();

	// Query Layer status functions 
	BOOL IsVisible(DocView* pDocView = NULL);
	BOOL IsLocked(); 
	BOOL IsActive(); 
	BOOL IsPrintable(); 
	BOOL IsBackground(); 
	BOOL IsOutline();
	BOOL IsGuide();
	BOOL IsPageBackground();
	String_256& GetLayerID(); 

	// Query Frame status functions
	BOOL IsOverlay();
	BOOL IsSolid();
	BOOL IsEdited();
	BOOL IsFrame();
	BOOL IsHiddenFrame();
	
	BOOL IsPseudoFrame();

	// These functions are used to return the real settings of some of the flags
	// This is because some IsX() functions take other things into account
	BOOL GetVisibleFlagState();
	BOOL GetLockedFlagState();

	// Has the layer got any renderable children
	BOOL HasLayerGotRenderableChildren();

	// When going through the layers to work out bounding boxes for the drawing
	// use this to determine if a layer should be included in the calculation
	BOOL IncludeLayerInBoundingCalcs();

	LayerStatus& GetLayerStatus(); 
	
public:
	// Set Layers status functions 
	void SetVisible(BOOL Status); 
	void SetLocked(BOOL Status); 
	void SetActive(BOOL Status); 
	void SetPrintable(BOOL Status); 
	void SetBackground(BOOL Status); 
	void SetOutline(BOOL Status);
	void SetGuide(BOOL Status);
	void SetPageBackground(BOOL Status);
	void SetLayerID(String_256& LayerID); 
	void SetLayerStatus(LayerStatus& Status); 
	
	// Set frame status functions
	void SetSolid(BOOL Status);
	void SetOverlay(BOOL Status);
	void SetEdited(BOOL Status);
	void SetFrame(BOOL Status);
	void SetHiddenFrame(BOOL Status);

	void EnsureUniqueLayerID(void);

public:
	void GetDebugDetails(StringBase* Str); 
	virtual UINT32 GetNodeSize() const; 			// Returns size of node in bytes

	// Returns the bounding rect of the layer inflated by the size of a blob
	virtual DocRect GetBlobBoundingRect();

	BOOL 		BelongsToDocument(Document* pDoc);
	Document* 	FindDocument();

	// Op permission functions
	virtual BOOL AllowOp(ObjChangeParam* pParam,BOOL SetOpPermissionState = TRUE,
												BOOL DoPreTriggerEdit = TRUE);
	virtual void SetOpPermission(OpPermissionState NewState,BOOL AndParents = FALSE);

	static BOOL ActiveLayerVisibleAndEditable;
	static INT32 EnableLayerCacheing;

	// Guide layer colour stuff
	DocColour* 	GetGuideColour();
	void		SetGuideColour(IndexedColour* pIndexedColour);
	void		MakeGuideColourDefault();
	DocColour	GetDefaultGuideColour();
	void 		ColourChanged(IndexedColour* pChangedColour);
	void 		ColourDeleted();

	virtual void PolyCopyNodeContents(NodeRenderable* pNodeCopy);

public:
	// Version 2 file format functions
	virtual BOOL WritePreChildrenWeb(BaseCamelotFilter* pFilter);
	virtual BOOL WritePreChildrenNative(BaseCamelotFilter* pFilter);
	virtual BOOL WritePostChildrenWeb(BaseCamelotFilter* pFilter);
	virtual BOOL WritePostChildrenNative(BaseCamelotFilter* pFilter);
	virtual BOOL CanWriteChildrenWeb(BaseCamelotFilter* pFilter);
	virtual BOOL CanWriteChildrenNative(BaseCamelotFilter* pFilter);

	virtual BOOL WriteBeginChildRecordsWeb(BaseCamelotFilter* pFilter);
	virtual BOOL WriteBeginChildRecordsNative(BaseCamelotFilter* pFilter);
	virtual BOOL WriteEndChildRecordsWeb(BaseCamelotFilter* pFilter);
	virtual BOOL WriteEndChildRecordsNative(BaseCamelotFilter* pFilter);

	virtual BOOL ReadPostChildrenWeb(BaseCamelotFilter* pFilter);
	virtual BOOL ReadPostChildrenNative(BaseCamelotFilter* pFilter);

	virtual UINT32 CountNumWritableWebLayers(BaseCamelotFilter* pFilter);

	virtual BOOL WriteAtomicNodesColourRefs (Node* pLayer, BaseCamelotFilter* pFilter);
	virtual PathName MakeExportLayerName(const PathName& path);

// WEBSTER - markn 15/1/97
// Moved CreateGuideLayer() from OpGuideline class
	static Layer* CreateGuideLayer();

public:
	// Frame related functions
	virtual BOOL WriteFrameLayerProperties(BaseCamelotFilter* pFilter);

	DWORD GetFrameDelay();
	void SetFrameDelay(const DWORD& Delay);

	// The quality level that the animation was captured at
	Quality GetCaptureQuality()								{ return m_CaptureQuality; }
	void SetCaptureQuality(const Quality& NewQuality)		{ m_CaptureQuality = NewQuality; }

	// Generated Bitmap related functions
	KernelBitmap * GetGeneratedBitmap();
	BOOL SetGeneratedBitmap(KernelBitmap * pBitmap);
	// Overide this so that our bitmaps get included in the counting process
	virtual KernelBitmap * EnumerateBitmaps(UINT32 Count);
	// Referenced Bitmap related functions
	KernelBitmap * GetReferencedBitmap();
	BOOL SetReferencedBitmap(KernelBitmap * pBitmap);

	// Functions for handling child nodes changing inside compound node,
	// We override this so we can mark the layer as edited
	virtual ChangeCode 	OnChildChange(ObjChangeParam* pParam);

private: 
	LayerStatus LayerSt; 	// All layer info which can be changed by the user in an undoable way

	BOOL Active;		// The active layer is the layer to which new objects get added. A single layer 
						// within a spread must always be active. 
	BOOL Visible; 		// This flag indicates if the layer should be rendered or not 
	BOOL Locked; 		// If a layer is locked then it should not be modifiable. i.e the user should 
						// not be able to change or add new objects to the layer.  
	BOOL Printable; 	// Indicates if the layer should be rendered to the printer
	BOOL Background; 	// ??? Prints at half tone (or something like this, need to find out !!) 
	BOOL Outline;		// Everything in this layer should be rendered in outlines (i.e. minimum quality)
	BOOL Guide;			// This layer is a Guide layer, and can contain guidelines
	
	BOOL m_PageBackground;	// This layer is the special page background layer and hence contains a
							// rectangle covering the page(s) of the spread with either a bitmap fill
							// or a colour applied.
	// Frame related system flags
	BOOL m_Overlay;		// Set if the previous layer is said to overlay the previous one rathar than
						// obscure it.
	BOOL m_Solid;		// A solid frame/layer is like a background layer in that it forms the background
						// for frames above it.
	BOOL m_Edited;		// Like the document flag, this says that the layer has been edited	and so the
						// new bitmap frame needs generating.
	BOOL m_Frame;		// This marks this frame as a special GIF animation frame layer.
	BOOL m_HiddenFrame;	// This marks this frame as a hidden frame layer which will not be saved into
						// the GIF animation but may take part in the rendering process of other frames.

	DWORD m_FrameDelay;	// The delay value for a GIF animation will be stored in each frame layer.

	Quality			m_CaptureQuality;	// The quality that the frame was captured at
	
	KernelBitmapRef m_GeneratedBitmap;	// The bitmap that has been generated to represent this frame/layer

	KernelBitmap * m_pReferencedBitmap;	// If there is a single bitmap on the frame of the correct type
										// then this will be used to represent this frame/layer

	DocColour* pGuideColour;
	IndexedColour* pIndexedGuideColour;

	// RGB value of default colour for guide layers
	static INT32 GuideLayerRed;
	static INT32 GuideLayerGreen;
	static INT32 GuideLayerBlue;

	// Message handler that keeps layers up-to-date
	static LayerMsgHandler* pLayerMsgHandler;

protected:              
	virtual Node* SimpleCopy(); 
	void CopyNodeContents(Layer* NodeCopy);
};


/********************************************************************************************

>	class LayerMsgHandler : public MessageHandler

	Author:		Mark_Neves (Xara Group Ltd) <camelotdev@xara.com>
	Created:	10/10/95
	Purpose:	Intercepts messages to ensure that layers are up to date
	
********************************************************************************************/

class LayerMsgHandler : public MessageHandler
{
public:
	CC_DECLARE_DYNAMIC( LayerMsgHandler ) 
	LayerMsgHandler();

protected:
	MsgResult Message(Msg* Msg);
};


#endif // INC_LAYER
