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



/*
*/ 

// Stardate 070494.10:25. Captain's Log - supplementary
// ====================================================
//
// 		Pirated fixed16 code to base fixed24 upon
// 		This was from revision 1.24 of fixed16.cpp
//												  Jason



#include "camtypes.h"

// this is a temporary kludge for the 32-bit MS compiler
#define	F24ASSIGN( it )		it
#define	F24ASSIGNTHIS		*this

// this is one as a fixed24 as a longword
const INT32 FIXED24_ONE = 0x01000000L;


#ifdef FULL_FIXED24
// the following functions are untouched copies from fixed16.cpp
// if you want them, you'll have to convert them to handle FIXED24's
// I've also removed the '>' from the help comments to avoid them being
// compiled into Camelot help

/********************************************************************************************

	static LPTCHAR LongToStringFormat( LPTCHAR p, INT32 value, BOOL leading, BOOL trailing)

	Author: 	Andy_Pennell (Xara Group Ltd) <camelotdev@xara.com>
	Created:	12/8/93
	Inputs:		Pointer to result, INT32 value and flags for leading & trailing spaces
	Outputs:	Buffer contains up to 5 digits of result
	Returns:	Returns the updated result pointer
	Purpose:	Converts low-value INT32s into decimal strings, either ASCII or Unicode
				depending on the size of TCHAR. value must be in range 0-999999 inclusive.
	Errors:		None.

********************************************************************************************/
/* Technical Note
	As the value can exceed 16-bits a little bit (99,999 is bigger than 65535) all calculations
	have to be done using INT32s. This is only a performance problem on 16-bit platforms, so it
	doesn't matter
*/

static LPTCHAR LongToStringFormat( LPTCHAR p, INT32 value, BOOL leading, BOOL trailing)
{
	INT32 digits = 5;
	INT32 tens = 10000L;

	while (--digits)
	{
		if (value >= tens)
		{
			// we need to output a non-zero digit

			*p++ = (TCHAR) ( TEXT('0') + (INT32)(value/tens) );
			value = value % tens;
		}
		else if (leading)
		{
			*p++ = TEXT('0');
		}
		// stop if zero result and no trailing zeros required
		if ( (value==0L) && (!trailing) )
			break;
		tens /= 10L;									// reduce divisor by 10
	}

	// final digit easy
	*p++ = (TCHAR) ( TEXT('0') + (INT32)value );

	// return next character
	return p;
}

/********************************************************************************************

	String FIXED16::ToString() const 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com>
	Created:	2/6/93 - rewritten in C 12/8/93 by Andy
	Inputs:		None.
	Outputs:	None
	Returns:	Returns a string representing the value of a FIXED16. Optional - sign, up
				to five digits of integer, then option period and five fractional digits
				then zero terminator (max 13 character result)
	Purpose:	Converts value of FIXED16 into a string representing it. The string will be
				ASCII or Unicode depending on the size of TCHAR
	Errors:		None.

********************************************************************************************/

void fixed16::ToString(String* out) const
{
	TCHAR s[13];
	LPTCHAR p = s;
	INT32 value = this->all;

	if (value < 0L)
	{
		*p++ = TEXT('-');								// negative values start with a -
		value = -value;									// ensure result positive
	}

	// integer part: no leading zeros, trailing zeros

	p = LongToStringFormat( p, value >> F16SHIFT, FALSE, FALSE);

	value &= 0xFFFF;									// get fractional part

	if (value)
	{
		*p++ = TEXT('.');								// decimal point needed
		value = MulDiv32By32( value, 100000L, 65536L);	// convert fractional part into INT32

		// factional part: leading zeros, no trailing
		p = LongToStringFormat( p, value, TRUE, FALSE );
	}

	*p = (TCHAR)0;										// null terminate
	
	*out = s;                                           // convert to proper String
}
#endif		// FULL_FIXED24




// Friend Functions - Basic Operators

/********************************************************************************************

>	FIXED24 operator+ (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Friend:		FIXED24
	Inputs:		Two values to be added together.
	Outputs:	None.
	Returns:	A FIXED24 with result of addition.
	Purpose:	Overloading the addition operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator+ (const fixed24& operand1, const fixed24& operand2)
{
	fixed24 result;
	              
	result.all = operand1.all + operand2.all;
	              
	return result;
}

/********************************************************************************************

>	FIXED24 operator- (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Friend:		FIXED24
	Inputs:		Two values to be subtracted from each other.
	Outputs:	None.
	Returns:	A FIXED24 with result of subtraction.
	Purpose:	Overloading the subtract operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator- (const fixed24& operand1, const fixed24& operand2)
{
	fixed24 result;
	              
	result.all = operand1.all - operand2.all;
	              
	return result;
}

/********************************************************************************************

>	FIXED24 operator- (const FIXED24& operand1)

	Author: 	Andy_Pennell (Xara Group Ltd) <camelotdev@xara.com>
	Created:	30/5/93
	Friend:		FIXED24
	Inputs:		A value to be negated.
	Outputs:	None.
	Returns:	A FIXED24 with result of negation.
	Purpose:	Overloading the negation operator for FIXED24s. Should be inline.
	Errors:		None.

********************************************************************************************/

fixed24 operator- (const fixed24& operand)
{
	fixed24 temp;

	temp.all = -operand.all;

	return temp;
}

/********************************************************************************************

>	FIXED24 operator* (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Mul().
	Friend:		FIXED24
	Inputs:		Two values to be multiplied togther.
	Outputs:	None.
	Returns:	A FIXED24 with result of multiplication.
	Purpose:	Overloading the multiplication operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator* (const fixed24& operand1, const fixed24& operand2)
{
	fixed24 result;
	              
	F24ASSIGN(result) = Fixed24Mul( operand1, operand2 );

	return result;
}

/********************************************************************************************

>	FIXED24 operator/ (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Div().
	Friend:		FIXED24
	Inputs:		Two values to be divided.
	Outputs:	None.
	Returns:	A FIXED24 with result of division.
	Purpose:	Overloading the division operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator/ (const fixed24& operand1, const fixed24& operand2)
{
	fixed24 result;
	              
	F24ASSIGN(result) = Fixed24Div( operand1, operand2 );

	return result;

}

/********************************************************************************************

>	FIXED24& operator>> (const FIXED24& operand1, UINT32 operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Friend:		FIXED24
	Inputs:		operand1 - value to be shifted.
				operand2 - shift count.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of ASR.
	Purpose:	Overloading the >> operator to mean shift right for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator>> (const fixed24& operand1, UINT32 operand2)
{
	fixed24 result;
	
	result.all = operand1.all >> operand2;
	
	return result;
}

/********************************************************************************************

>	FIXED24 operator>> (const FIXED24& operand1, UINT32 operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Friend:		FIXED24
	Inputs:		operand1 - value to be shifted.
				operand2 - shift count.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of ASL.
	Purpose:	Overloading the << operator to mean shift left for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 operator<< (const fixed24& operand1, UINT32 operand2)
{
	fixed24 result;
	
	result.all = operand1.all << operand2;
	
	return result;
}

// Relational operators

/********************************************************************************************

>	INT32 operator== (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the equality operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator== (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all == operand2.all;
}

/********************************************************************************************

>	INT32 operator== (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	2/6/93
	Friend:		FIXED24
	Inputs:		FIXED24
				INT8
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the equality operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator== (const fixed24& operand1, const short operand2) 
{
	return operand1.all == SHORT_FIXED24( operand2 );
}

/********************************************************************************************

>	INT32 operator!= (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the inequality operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator!= (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all != operand2.all;
}

/********************************************************************************************

>	INT32 operator!= (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		FIXED24
				short
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the inequality operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator!= (const fixed24& operand1, const short operand2) 
{
	return operand1.all != SHORT_FIXED24( operand2 );
}

/********************************************************************************************

>	INT32 operator> (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the greater-than operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator> (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all > operand2.all;
}

/********************************************************************************************

>	INT32 operator> (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		FIXED24
				short
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the greater-than operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator> (const fixed24& operand1, const short operand2) 
{
	return operand1.all > SHORT_FIXED24(operand2);
}

/********************************************************************************************

>	INT32 operator< (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the less-than operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator< (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all < operand2.all;
}

/********************************************************************************************

>	INT32 operator< (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		FIXED24
				short
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the less-than operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator< (const fixed24& operand1, const short operand2) 
{
	return operand1.all < SHORT_FIXED24( operand2 );
}

/********************************************************************************************

>	INT32 operator>= (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the greater-than-equal-to operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator>= (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all >= operand2.all;
}

/********************************************************************************************

>	INT32 operator>= (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		FIXED24
				short
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the greater-than-equal-to operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator>= (const fixed24& operand1, const short operand2) 
{
	return operand1.all >= SHORT_FIXED24( operand2 );
}

/********************************************************************************************

>	INT32 operator<= (const FIXED24& operand1, const FIXED24& operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		Two FIXED24s to be compared.
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the less-than-equal-to operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator<= (const fixed24& operand1, const fixed24& operand2) 
{
	return operand1.all <= operand2.all;
}

/********************************************************************************************

>	INT32 operator<= (const FIXED24& operand1, const short operand2) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	5/5/93
	Friend:		FIXED24
	Inputs:		FIXED24
				short
	Outputs:	None.
	Returns:	0 - False
				1 - True
	Purpose:	Overloading the less-than-equal-to operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

INT32 operator<= (const fixed24& operand1, const short operand2) 
{
	return operand1.all <= SHORT_FIXED24( operand2 );
}

// Assignment operators

/********************************************************************************************

>	FIXED24& FIXED24::operator= (const INT32 operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		operand to be  assigned - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the assignment operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator= (const INT32 operand)
{
	this->all = SHORT_FIXED24( operand );
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator= (const double operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	DoubleToFixed24().
	Inputs:		operand to be  assigned - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the assignment operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator= (const double operand)
{ 
	F24ASSIGNTHIS = DoubleToFixed24(operand);
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator+= (const FIXED24& operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be incremented
				operand to be added - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the plus-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator+= (const fixed24& operand)
{ 
	this->all += operand.all;
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator+= (const short operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be incremented
				operand to be added - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the plus-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator+= (const short operand)
{ 
	this->all += SHORT_FIXED24( operand );
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator-= (const FIXED24& operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be decremented
				operand to be subtracted - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the minus-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator-= (const fixed24& operand)
{
	this->all -= operand.all;
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator-= (const short operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be decremented
				operand to be subtracted - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the minus-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24& fixed24::operator-= (const short operand)
{
	this->all -= SHORT_FIXED24( operand );
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator*= (const FIXED24& operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Mul().
	Inputs:		lvalue to be multiplied
				multiplier - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the times-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
 
fixed24& fixed24::operator*= (const fixed24& operand)
{
	F24ASSIGNTHIS = Fixed24Mul(*this, operand);
	
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator*= (const short operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Mul().
	Inputs:		lvalue to be multiplied
				multiplier - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the times-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
 
fixed24& fixed24::operator*= (const short operand)
{                                  
	fixed24 temp;
	
	temp.all = SHORT_FIXED24( operand );
	
	F24ASSIGNTHIS = Fixed24Mul(*this, temp);
	
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator/= (const FIXED24& operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Div().
	Inputs:		lvalue to be divided
				divisor - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the divide-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
 
fixed24& fixed24::operator/= (const fixed24& operand)
{
	F24ASSIGNTHIS = Fixed24Div(*this, operand);
	
	return *this;
}
 
/********************************************************************************************

>	FIXED24& FIXED24::operator/= (const short operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	SeeAlso:	Fixed24Div().
	Inputs:		lvalue to be divided
				divisor - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the divide-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
 
fixed24& fixed24::operator/= (const short operand)
{
	fixed24 temp;
	
	temp.all = SHORT_FIXED24( operand );
	
	F24ASSIGNTHIS = Fixed24Div(*this, temp);
	
	return *this;
}
 
/********************************************************************************************

>	FIXED24& FIXED24::operator<<= (const UINT32 operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be shifted
				shift count - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the left-shift-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
 
fixed24& fixed24::operator<<= (const UINT32 operand)
{
	this->all <<= operand;
	return *this;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator>>= (const UINT32 operand) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		lvalue to be shifted
				shift count - rhs of assignment.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of assignment.
	Purpose:	Overloading the right-shift-equals operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
  
fixed24& fixed24::operator>>= (const UINT32 operand)
{
	this->all >>= operand;
	return *this;
} 

// Increment FIXED24::operators

/********************************************************************************************

>	FIXED24& FIXED24::operator++ () 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		None.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of increment.
	Purpose:	Overloading the prefix plus-plus operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
  
fixed24& fixed24::operator++ ()							// prefix
{
	this->all += FIXED24_ONE;
	return *this;
}
							
/********************************************************************************************

>	FIXED24 FIXED24::operator++ (INT32 dummy) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		dummy value to distinguish between pre- and post-fix application.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of increment.
	Purpose:	Overloading the postfix plus-plus operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
  
fixed24 fixed24::operator++ (INT32 dummy)   				// postfix 
{                
	fixed24 result = *this;
	
	this->all += FIXED24_ONE;
	
	return result;
}

/********************************************************************************************

>	FIXED24& FIXED24::operator-- () 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		None.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of decrement.
	Purpose:	Overloading the prefix minus-minus operator for FIXED24s.
	Errors:		None.

********************************************************************************************/
  
fixed24& fixed24::operator-- ()							// prefix 
{
	this->all -= FIXED24_ONE;
	return *this;
}

/********************************************************************************************

>	FIXED24 FIXED24::operator-- (INT32 dummy) 

	Author: 	Mario_Shamtani (Xara Group Ltd) <camelotdev@xara.com> (Jason Williams)
	Created:	27/4/93
	Inputs:		dummy value to distinguish between pre- and post-fix application.
	Outputs:	None.
	Returns:	A reference to an FIXED24 with result of decrement.
	Purpose:	Overloading the postfix minus-minus operator for FIXED24s.
	Errors:		None.

********************************************************************************************/

fixed24 fixed24::operator-- (INT32 dummy)				// postfix 
{
	fixed24 result = *this;
	
	this->all -= FIXED24_ONE;
	
	return result;
}
	
