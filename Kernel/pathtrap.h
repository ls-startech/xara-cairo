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

// Definition of Path Trapezoid classes (used in stroke providing)

#ifndef INC_PATHTRAP
#define INC_PATHTRAP

#include "pathproc.h"


// Forward declarations
class ProcessPathToTrapezoids;
class TrapsList;

/******************************************************************************************

>	enum TrapTravelType

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	13/1/97

	Purpose:	Describes how (if at all) to record "travel" in trapezoid lists

	SeeAlso:	ProcessPathToTrapezoids::Process

******************************************************************************************/

typedef enum
{
	TrapTravel_None,			// Don't record trapezoid travel at all
	TrapTravel_Parametric,		// Record travel as relative 0.0 to 1.0 parametric range
	TrapTravel_Millipoint		// Record travel as absolute millipoints distance
} TrapTravelType;



/******************************************************************************************

>	enum TrapJoinType

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	15/1/97

	Purpose:	Describes what kind of join (if any) the trapezoid between the "previous"
				TrapEdge and "this" TrapEdge represents. This is used to mark all traps
				lying within joins, and to indicate the join style to the stroker, as
				rounded joins are "smoothed" by the stroker, while mitred/bevelled joins
				must always produce straight lines.

******************************************************************************************/

typedef enum
{
	TrapJoin_None,
	TrapJoin_Round,
	TrapJoin_MitredOrBevelled
} TrapJoinType;



/******************************************************************************************

>	class NormCoord : public CC_CLASS_MEMDUMP

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/12/96

	Purpose:	Normalised vector/coordinate
				When normalised, the vector is always of unit length

	Notes:		The entire class is inlined to keep these simple operations efficient

******************************************************************************************/

class NormCoord : public CCObject
{
CC_DECLARE_DYNCREATE(NormCoord);

public:		// Constructors
	NormCoord()								{ x = y = 0.0; };
	NormCoord(double X, double Y)			{ x = X; y = Y; };
	NormCoord(NormCoord &Other)				{ x = Other.x; y = Other.y; };

public:
	// Normalise this vector to be of unit length
	void Normalise(void)
	{
		double Normalise = sqrt(x*x + y*y);
		if (Normalise == 0.0)
		{
			TRACE( _T("** NormCoord::Normalise - Zero-length vector (%f)\n"), Normalise);
		}
		else
		{
			Normalise = 1/Normalise;
			x *= Normalise;
			y *= Normalise;
		}
	}

	// Averaging of 2 vectors. Both input vectors must be normalised
	// The result overwrites "this" NormCoord, and is always normalised
	// If the vectors are exactly equal and opposite, returns a perpendicular to "this"
	inline void Average(NormCoord &C1, NormCoord &C2)
	{
		if (C1.x + C2.x == 0.0 && C1.y + C2.y == 0.0)
		{
			double Temp = C1.x;
			x = C1.y;
			y = -Temp;
		}
		else
		{
			x = (C1.x + C2.x) / 2.0;
			y = (C1.y + C2.y) / 2.0;
			Normalise();
		}
	}

	inline double GetLength()
	{
		return sqrt(x * x + y * y);
	}

	// Make a normalised direction vector for a line from P1 to P2
	inline void SetFromLine(DocCoord &P1, DocCoord &P2)
	{
		x = P1.x - P2.x;
		y = P1.y - P2.y;
		Normalise();
	}

	// Make a normalised perpendicular vector (Normal) for a line from P1 to P2
	inline void SetNormalToLine(DocCoord &P1, DocCoord &P2)
	{
		x = P1.y - P2.y;
		y = -(P1.x - P2.x);
		Normalise();
	}

	// Calculate the dot product of this vector and another. Both inputs must already
	// have been normalised, or the result will be bogus.
	inline double DotProduct(NormCoord &Other)
	{
		return( (x * Other.x) + (y * Other.y) );
	}

	// Assignment operator
	inline NormCoord& operator=(const NormCoord &Other)
	{
		x = Other.x;
		y = Other.y;
		return(*this);
	}

public:
	double x;
	double y;
};



/******************************************************************************************

>	struct TrapEdge

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/12/96

	Purpose:	Structure defining a trapezoid edge definition.
				A TrapEdge records the following information:

					Centre   - A centerline point for the edge, which lies on the original path

					Normal   - A normal vector to the curve at this centre point

					Position - The position of this centre point along the original path
							   During construction of a TrapEdgeList this is a physical
							   MILLIPOINT distance, but at the end of the process it is
							   converted into a parametric position representation in
							   the range 0.0 to 1.0

******************************************************************************************/

typedef struct
{
	DocCoord Centre;			// Centreline position on the destination path
	NormCoord Normal;			// Normal to the curve at this point (points toward "left" side)
	NormCoord Normal2;			// Tertiary normal, mainly used by bevtrap
	double Position;			// The fractional position of this trap along the destination path
	TrapJoinType PrevTrapJoin;	// Indicates what type of join (if any) the previous trap belongs to
} TrapEdge;



/******************************************************************************************

>	class TrapEdgeList : public CC_CLASS_MEMDUMP

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/12/96

	Purpose:	Simple class to store a list of trapezoid edges (TrapEdge structures)
				Each source sub-path will be converted to a separate TrapEdgeList

	Notes:		Sorry, Jim, but I've had to use inlining to keep common operations efficient

******************************************************************************************/

class TrapEdgeList : public CCObject
{
CC_DECLARE_DYNCREATE(TrapEdgeList);

public:
	TrapEdgeList(TrapsList *pParent = NULL);
	~TrapEdgeList();
			// Default constructor & destructor

	inline UINT32 GetNumEdges(void)				{ return(Used); };
			// Determine the total number of elements in the list

	inline TrapEdge *GetTrapEdge(UINT32 index)	{ ERROR3IF(index >= Used, "Out of range"); return(&pEdges[index]); };
			// Retrieve the given element of the list

	inline TrapEdge *GetLastTrapEdge(void)		{ return((Used == 0) ? NULL : &pEdges[Used-1]); };
			// Retrieve the last element in the list

	UINT32 FindTrapEdge(double Position, UINT32 LoIndex = 0, UINT32 HiIndex = 0);
			// Finds the last trapezoid edge with a position less than the given position.
			// LoIndex/HiIndex indicate the bouds of the array to start searching within (it is
			// not limited to the bounds - they just make searches in small regions much faster)

	BOOL AddEdge(DocCoord *pPoint, TrapJoinType JoinType = TrapJoin_None);
			// Appends a new TrapEdge to the list

	BOOL ProcessEdgeNormals(ProcessPathToTrapezoids *pProcessor);
	BOOL ProcessEdgePositions(TrapTravelType TravelType);
			// Post-processes the entire (completed) list to fill in all the details

	inline double GetPathLength(void)			{ return PathLength; };
			// Returns the length (in millipoints) of the path represented in this TrapEdgelist

protected:
	BOOL ExpandArray(void);
			// Expands the array to allow for more entries

protected:
	UINT32 Used;				// Number of entries used (index of first free space in array)
	UINT32 CurrentSize;		// Number of entries allocated for the array
	TrapEdge *pEdges;		// Pointer to the edge array

	double PathLength;		// Length (travel) of the represented (sub)path in millipoints

	TrapsList *pParentList;	// Points to the TrapsList in which we reside
};



/******************************************************************************************

>	class TrapsList : public CC_CLASS_MEMDUMP

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/12/96

	Purpose:	Simple class containing a list of TrapEdgeLists representing a complex path
				Each TrapEdgeList represents a sub-path of the overall source path.

	Notes:		Sorry, Jim, but I've had to use inlining to keep common operations efficient

******************************************************************************************/

class TrapsList : public CCObject
{
CC_DECLARE_DYNCREATE(TrapsList)

public:
	TrapsList(INT32 Repeat = 0);
	~TrapsList();

	inline UINT32 GetNumTraps(void)			{ return(Used); };
			// Determine the total number of elements in the list

	inline TrapEdgeList *GetTrapEdgeList(UINT32 index)
									{ ERROR3IF(index >= Used, "Out of range"); return(pTraps[index]); };
			// Retrieve the given element of the list

	inline TrapEdgeList *GetLastTrapEdgeList(void)
									{ return((Used == 0) ? NULL : pTraps[Used-1]); };
			// Retrieve the last element in the list

	TrapEdgeList *AddEdgeList(void);
			// Adds a new TrapEdgeList to the list, returning it for you to fill in

	BOOL PostProcessLists(ProcessPathToTrapezoids *pProcessor, TrapTravelType TravelType);
			// Post-processes all trapezoid lists to complete calculation of all values

	inline INT32 GetRepeatLength(void)		{ return(RepeatLength); };
			// Retrieve the length of repeating stroke sections (or 0 if there is no repeat)

protected:
	BOOL ExpandArray(void);
			// Expands the array to allow for more entries

protected:
	UINT32 Used;				// Number of entries used (index of first free space in array)
	UINT32 CurrentSize;		// Number of entries allocated for the array
	TrapEdgeList **pTraps;	// Pointer to the EdgeList array

	INT32 RepeatLength;		// Length (in millipoints) after which the stroke should repeat
							// (or 0 if the stroke should only repeat once)
};



/******************************************************************************************

>	class ProcessPathToTrapezoids : public ProcessPath

	Author:		Jason_Williams (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/12/96

	Purpose:	Process a path to produce a trapezoid list suitable for variable-
				width or vector-brushed stroke generation.

******************************************************************************************/

class ProcessPathToTrapezoids : public ProcessPath
{
friend class TrapEdgeList;

public:			// Construction & Invocation
	ProcessPathToTrapezoids(const double flat);
			// Construct

	virtual BOOL Init(Path* pSource, TrapsList *pOutputList);
			// Initialise ready for use

	virtual BOOL Process(const ProcessFlags &PFlags,
						 TrapTravelType TravelType, JointType JoinStyle = RoundJoin);
			// Convert the source into trapezoidal strokes in the given style


public:			// Overridden virtuals called by base class
	virtual BOOL NewPoint(PathVerb Verb, DocCoord *pCoord);
	virtual BOOL CloseElement(BOOL done, PathVerb Verb, INT32 Index);
	virtual void CloseFigure(void);


protected:
	BOOL CalculateMitreIntersection(DocCoord *p1, DocCoord *p2, DocCoord *p3,
									double *pMitreRatio = NULL);

private:
	TrapsList	*pTraps;			// List of trapezoids being created
	BOOL		PointFollowsJoin;	// TRUE when a NewPoint immediately follows a join (knot)
	JointType	JoinType;			// Indicates the join style we wish to use
	INT32		RepeatLength;		// 0 (no repeat), or the length of repeating sections (in millipoints)
	DocCoord	LastPoint;			// Used to detect & remove coincident points from the flattened path
};


#endif

