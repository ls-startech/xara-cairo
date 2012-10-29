// This class contains all the file deflating code

#include "camtypes.h"

#include "zdeflate.h"
#include "zstream.h"

// This is not compulsory, but you may as well put it in so that the correct version
// of your file can be registered in the .exe
DECLARE_SOURCE("$Revision: 1282 $");

// An implement to match the Declare in the .h file.
// If you have many classes, it is recommended to place them all together, here at the start of the file
CC_IMPLEMENT_MEMDUMP(ZipDeflate, CC_CLASS_MEMDUMP)
CC_IMPLEMENT_MEMDUMP(DeflateState, CC_CLASS_MEMDUMP)

// This will get Camelot to display the filename and linenumber of any memory allocations
// that are not released at program exit

#define new CAM_DEBUG_NEW

// This is all based on:-
 
/* deflate.c -- compress data using the deflation algorithm
 * Copyright (C) 1995 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h 
 */

/*
 *  ALGORITHM
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many people for bug reports and testing.
 *
 *  REFERENCES
 *
 *      Deutsch, L.P.,"'Deflate' Compressed Data Format Specification".
 *      Available in ftp.uu.net:/pub/archiving/zip/doc/deflate-1.1.doc
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 */

#define NIL 0
/* Tail of hash chains */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */

//typedef INT32 (*compress_func)(DeflateState *s, INT32 flush);
enum compress_func {Z_deflate_stored, Z_deflate_fast, Z_deflate_slow};
/* Compressing function */

typedef struct config_s {
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
   compress_func func;
} config;

	config configuration_table[10] = {
	/*      good lazy nice chain */
	/* 0 */ {0,    0,  0,    0, Z_deflate_stored},  /* store only */
	/* 1 */ {4,    4,  8,    4, Z_deflate_fast}, /* maximum speed, no lazy matches */
	/* 2 */ {4,    5, 16,    8, Z_deflate_fast},
	/* 3 */ {4,    6, 32,   32, Z_deflate_fast},

	/* 4 */ {4,    4, 16,   16, Z_deflate_slow},  /* lazy matches */
	/* 5 */ {8,   16, 32,   32, Z_deflate_slow},
	/* 6 */ {8,   16, 128, 128, Z_deflate_slow},
	/* 7 */ {8,   32, 128, 256, Z_deflate_slow},
	/* 8 */ {32, 128, 258, 1024, Z_deflate_slow},
	/* 9 */ {32, 258, 258, 4096, Z_deflate_slow}}; /* maximum compression */

	/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
	 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
	 * meaning.
	 */

#define EQUAL 0
/* result of memcmp for equal strings */

struct static_tree_desc_s {INT32 dummy;}; /* for buggy compilers */

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(s,h,c) (h = (((h)<<s->hash_shift) ^ (c)) & s->hash_mask)


/* ===========================================================================
 * Insert string str in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of str are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, str, match_head) \
   (UPDATE_HASH(s, s->ins_h, s->window[(str) + (MIN_MATCH-1)]), \
    s->prev[(str) & s->w_mask] = match_head = s->head[s->ins_h], \
    s->head[s->ins_h] = (Pos)(str))

/* ===========================================================================
 * Initialize the hash table (avoiding 64K overflow for 16 bit systems).
 * prev[] will be initialized on the fly.
 */
#define CLEAR_HASH(s) \
    s->head[s->hash_size-1] = NIL; \
    zmemzero((charf *)s->head, (unsigned)(s->hash_size-1)*sizeof(*s->head));


/* ----------------------------- DeflateState class -------------------------------------- */

/********************************************************************************************

>	DeflateState::DeflateState()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	34/05/95
	Purpose:	DeflateState constructor.

********************************************************************************************/

DeflateState::DeflateState()
{
    strm = NULL;      		/* pointer back to this zlib stream */
    pending_buf = NULL;   	/* output still pending */
    pending_out = NULL;   	/* next pending byte to output to the stream */

	window = NULL;			// sliding window usually 2*wSize
    prev = NULL;			// Link to older string with same hash index.
    head = NULL;			// Heads of the hash chains or NULL.
	l_buf = NULL;			// buffer for literals or lengths
	d_buf = NULL;			// buffer for distance

	status = 0;
	pending = 0;
	noheader = 0;
	data_type = Z_UNKNOWN;
	method = DEFLATED;
	last_flush = 0;
	w_size = 0;
	w_bits = 0;
	w_mask = 0;
	window_size = 0;
	ins_h = 0;
	hash_size = 0;
	hash_bits = 0;
	hash_mask = 0;
	hash_shift = 0;
	block_start = 0;
	match_length = 0;
	prev_match = 0;
	match_available = 0;
	match_start = 0;
	block_start = 0;
	lookahead = 0;
	prev_length = 0;
	max_chain_length = 0;
	max_lazy_match = 0;
	level = 0;
	nice_match = 0;
	heap_len = 0;
	heap_max = 0;
	nice_match = 0;
	lit_bufsize = 0;
	last_lit = 0;
	opt_len = 0;
	static_len = 0;
	compressed_len = 0;
	matches = 0;
	last_eob_len = 0;
#ifdef DEBUG
	bits_sent = 0;
#endif
	bi_buf = 0;
	bi_valid = 0;
}	

/********************************************************************************************

>	DeflateState::~DeflateState()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	34/05/95
	Purpose:	DeflateState destructor.

********************************************************************************************/

DeflateState::~DeflateState()
{
	
}	

/* ---------------------------------- ZipDeflate class -------------------------------------- */

/********************************************************************************************

>	ZipDeflate::ZipDeflate()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	34/05/95
	Purpose:	ZipDeflate constructor.

********************************************************************************************/

ZipDeflate::ZipDeflate()
{
}	

/********************************************************************************************

>	ZipDeflate::~ZipDeflate()

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	34/05/95
	Purpose:	ZipDeflate destructor.

********************************************************************************************/

ZipDeflate::~ZipDeflate()
{
	
}	

/********************************************************************************************

>	INT32 ZipDeflate::Init(ZStream *strm, INT32 level)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Inputs:		s		the zip stream to use
				level	usually Z_DEFAULT_COMPRESSION, or between 1 and 9. 1 best speed
						9 best compression
	Purpose:	Initializes the internal stream state for compression. The fields
				zalloc, zfree and opaque must be initialized before by the caller.
				If zalloc and zfree are set to Z_NULL, deflateInit updates them to
				use default allocation functions.

				The compression level must be Z_DEFAULT_COMPRESSION, or between 1 and 9:
				1 gives best speed, 9 gives best compression. Z_DEFAULT_COMPRESSION requests
				a default compromise between speed and compression (currently equivalent
				to level 6).

				deflate Init returns Z_OK if success, Z_MEM_ERROR if there was not
				enough memory, Z_STREAM_ERROR if level is not a valid compression level.
				msg is set to null if there is no error message.  deflateInit does not
				perform any compression: this will be done by deflate().

********************************************************************************************/

INT32 ZipDeflate::Init(ZStream *strm, INT32 level)
{
	// Just call the main init with default parameters
    return Init(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
				//version, stream_size);
    /* To do: ignore strm->next_in if we use it as window */
}

/********************************************************************************************

>	INT32 ZipDeflate::Init(ZStream *strm, INT32 level, INT32 method, INT32 windowBits,
					 	 INT32 memLevel, INT32 strategy)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Inputs:		strm		the zip stream to use
				level		usually Z_DEFAULT_COMPRESSION, or between 1 and 9. 1 best speed
							9 best compression
				method		always DEFLATED at present (For future expansion)
				windowBits	the base two logarithm of the window size, range 8..15
				memLevel	memory to allocate for the internal compression state 1..9 (8 default)
				strategy	used to tune the compressor to the current type of data
	Purpose:	This is another version of deflate Init with more compression options. The
				fields next_in, zalloc and zfree must be initialized before by the caller.

				The method parameter is the compression method. It must be 8 in this
				version of the library. (Method 9 will allow a 64K history buffer and
				partial block flushes.)

				The windowBits parameter is the base two logarithm of the window size
				(the size of the history buffer).  It should be in the range 8..15 for this
				version of the library (the value 16 will be allowed for method 9). Larger
				values of this parameter result in better compression at the expense of
				memory usage. The default value is 15 if deflateInit is used instead.

				The memLevel parameter specifies how much memory should be allocated
				for the internal compression state. memLevel=1 uses minimum memory but
				is slow and reduces compression ratio; memLevel=9 uses maximum memory
				for optimal speed. The default value is 8. See zconf.h for total memory
				usage as a function of windowBits and memLevel.

				The strategy parameter is used to tune the compression algorithm. Use
				the value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data
				produced by a filter (or predictor), or Z_HUFFMAN_ONLY to force Huffman
				encoding only (no string match).  Filtered data consists mostly of small
				values with a somewhat random distribution. In this case, the
				compression algorithm is tuned to compress them better. The strategy
				parameter only affects the compression ratio but not the correctness of
				the compressed output even if it is not set appropriately.

				If next_in is not null, the library will use this buffer to hold also
				some history information; the buffer must either hold the entire input
				data, or have at least 1<<(windowBits+1) bytes and be writable. If next_in
				is null, the library will allocate its own history buffer (and leave next_in
				null). next_out need not be provided here but must be provided by the
				application for the next call of deflate().

				If the history buffer is provided by the application, next_in must
				must never be changed by the application since the compressor maintains
				information inside this buffer from call to call; the application
				must provide more input only by increasing avail_in. next_in is always
				reset by the library in this case.

				This deflate Init returns Z_OK if success, Z_MEM_ERROR if there was
				not enough memory, Z_STREAM_ERROR if a parameter is invalid (such as
				an invalid method). msg is set to null if there is no error message.
				This deflate Init does not perform any compression: this will be done by
				deflate().

********************************************************************************************/

INT32 ZipDeflate::Init(ZStream *strm, INT32 level, INT32 method, INT32 windowBits,
					 INT32 memLevel, INT32 strategy)
					 //const char *version, INT32 stream_size)
{
    ushf *overlay;
    /* We overlay pending_buf and d_buf+l_buf. This works since the average
     * output size for (length,distance) codes is <= 24 bits.
     */
	
	// First, check if the stream passed in is valid or not
    //if (version == Z_NULL || version[0] != ZLIB_VERSION[0] || stream_size != sizeof(ZStream))
	//{
	//	return Z_VERSION_ERROR;
    //}
	
	if (strm == NULL) return Z_STREAM_ERROR;

    DeflateState *s = NULL;
    INT32 noheader = 0;

    strm->msg = NULL;
    if (strm->zalloc == NULL)
	{
		strm->zalloc = GZipFile::zcalloc;
		strm->opaque = 0; // (voidpf)0;
	}
    if (strm->zfree == NULL) strm->zfree = GZipFile::zcfree;

    if (level == Z_DEFAULT_COMPRESSION)
    	level = 6;

    if (windowBits < 0)
    { /* undocumented feature: suppress zlib header */
        noheader = 1;
        windowBits = -windowBits;
    }

    if (memLevel < 1 || memLevel > MAX_MEM_LEVEL || method != Z_DEFLATED ||
        windowBits < 8 || windowBits > 15 || level < 0 || level > 9 ||
		strategy < 0 || strategy > Z_HUFFMAN_ONLY)
	{
        return Z_STREAM_ERROR;
    }

    //s = ( DeflateState *) ZALLOC(strm, 1, sizeof( DeflateState));
    s = new DeflateState;
    if (s == NULL)
		return Z_MEM_ERROR;

	strm->De_state = s;
   	s->strm = strm;

    s->noheader = noheader;
    s->w_bits = windowBits;
    s->w_size = 1 << (s->w_bits);
    s->w_mask = s->w_size - 1;

    s->hash_bits = memLevel + 7;
    s->hash_size = 1 << (s->hash_bits);
    s->hash_mask = s->hash_size - 1;
    s->hash_shift =  ((s->hash_bits+MIN_MATCH-1)/MIN_MATCH);

    s->window = (Byte*) ZALLOC(strm, s->w_size, 2*sizeof(Byte));
    s->prev   = (Pos*)  ZALLOC(strm, s->w_size, sizeof(Pos));
    s->head   = (Pos*)  ZALLOC(strm, s->hash_size, sizeof(Pos));

    s->lit_bufsize = 1 << (memLevel + 6); /* 16K elements by default */

    overlay = (ushf *) ZALLOC(strm, s->lit_bufsize, sizeof(ush)+2);
    s->pending_buf = (uchf *) overlay;

    if (s->window == NULL || s->prev == NULL || s->head == NULL ||
        s->pending_buf == NULL)
    {
		//strm->msg = ERR_MSG(Z_MEM_ERROR);
        End(strm);
        return Z_MEM_ERROR;
    }

    s->d_buf = overlay + s->lit_bufsize/sizeof(ush);
    s->l_buf = s->pending_buf + (1+sizeof(ush))*s->lit_bufsize;

    s->level = level;
    s->strategy = strategy;
    s->method = (Byte)method;

    return Reset(strm);
}

/********************************************************************************************

>	INT32 ZipDeflate::SetDictionary (ZStream *strm, const Bytef *dictionary, uInt dictLength)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	03/05/96
	Inputs:		s			the zip stream to use
				dictionary	pointer to the dictionary to use
				dictlength	length of dictionary
	Returns:	returns Z_OK if success, Z_STREAM_ERROR if the source stream state was inconsistent
	Purpose:	Initializes the compression dictionary (history buffer) from the given
				byte sequence without producing any compressed output. This function must
			   be called immediately after deflateInit or deflateInit2, before any call
			   of deflate. The compressor and decompressor must use exactly the same
			   dictionary (see inflateSetDictionary).
				 The dictionary should consist of strings (byte sequences) that are likely
			   to be encountered later in the data to be compressed, with the most commonly
			   used strings preferably put towards the end of the dictionary. Using a
			   dictionary is most useful when the data to be compressed is short and
			   can be predicted with good accuracy; the data can then be compressed better
			   than with the default empty dictionary. In this version of the library,
			   only the last 32K bytes of the dictionary are used.
				 Upon return of this function, strm->adler is set to the Adler32 value
			   of the dictionary; the decompressor may later use this value to determine
			   which dictionary has been used by the compressor. (The Adler32 value
			   applies to the whole dictionary even if only a subset of the dictionary is
			   actually used by the compressor.)

				 deflateSetDictionary returns Z_OK if success, or Z_STREAM_ERROR if a
			   parameter is invalid (such as NULL dictionary) or the stream state
			   is inconsistent (for example if deflate has already been called for this
			   stream). deflateSetDictionary does not perform any compression: this will
			   be done by deflate(). 

				New for version 0.99

********************************************************************************************/

INT32 ZipDeflate::SetDictionary(ZStream *strm, const Bytef *dictionary, uInt dictLength)
{
    DeflateState *s;
    uInt length = dictLength;
    uInt n;
    IPos hash_head = 0;

    if (strm == Z_NULL || strm->De_state == Z_NULL || dictionary == Z_NULL ||
        strm->De_state->status != INIT_STATE)
		return Z_STREAM_ERROR;

    s = strm->De_state;
    strm->adler = GZipFile::adler32(strm->adler, dictionary, dictLength);

    if (length < MIN_MATCH)
		return Z_OK;
    
	if (length > MAX_DIST(s))
	{
		length = MAX_DIST(s);
		dictionary += dictLength - length;
    }
    zmemcpy((charf *)s->window, dictionary, length);
    s->strstart = length;
    s->block_start = (INT32)length;

    /* Insert all strings in the hash table (except for the last two bytes).
     * s->lookahead stays null, so s->ins_h will be recomputed at the next
     * call of fill_window.
     */
    s->ins_h = s->window[0];
    UPDATE_HASH(s, s->ins_h, s->window[1]);
    for (n = 0; n <= length - MIN_MATCH; n++)
	{
		INSERT_STRING(s, n, hash_head);
    }
    if (hash_head) hash_head = 0;  /* to make compiler happy */

    return Z_OK;
}


/********************************************************************************************

>	INT32 ZipDeflate::Reset(ZStream *strm)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Inputs:		s	the zip stream to use
	Returns:	returns Z_OK if success, Z_STREAM_ERROR if the source stream state was inconsistent
	Purpose:    This function is equivalent to deflate End followed by deflateInit,
				but does not free and reallocate all the internal compression state.
				The stream will keep the same compression level and any other attributes
				that may have been set by deflate Init.

				deflate Reset returns Z_OK if success, or Z_STREAM_ERROR if the source
				stream state was inconsistent (such as zalloc or state being NULL).

		

********************************************************************************************/

INT32 ZipDeflate::Reset(ZStream *strm)
{
    DeflateState *s;
    
    if (strm == Z_NULL || strm->De_state == Z_NULL || strm->zalloc == Z_NULL || strm->zfree == Z_NULL)
		return Z_STREAM_ERROR;

    strm->total_in = strm->total_out = 0;
    strm->msg = NULL; /* use zfree if we ever allocate msg dynamically */
    strm->data_type = Z_UNKNOWN;

	s = strm->De_state;
    s->pending = 0;
    s->pending_out = s->pending_buf;

    if (s->noheader < 0)
	{
        s->noheader = 0; /* was set to -1 by deflate(..., Z_FINISH); */
    }
    s->status = s->noheader ? BUSY_STATE : INIT_STATE;
    strm->adler = 1;
    s->last_flush = Z_NO_FLUSH;

    _tr_init(s);
    lm_init(s);

    return Z_OK;
}

/********************************************************************************************

>	INT32 ZipDeflate::Params(ZStream *strm, INT32 level, INT32 strategy)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	03/05/95
	Inputs:		
	Returns:	returns Z_OK if success, Z_STREAM_ERROR if the source stream state was inconsistent
	Purpose:    
				New for version 0.99
				Dynamically update the compression level and compression strategy.
				This can be used to switch between compression and straight copy of
				the input data, or to switch to a different kind of input data requiring
				a different strategy. If the compression level is changed, the input
				available so far is compressed with the old level (and may be flushed);
				the new level will take effect only at the next call of deflate().

				Before the call of deflateParams, the stream state must be set as for
				a call of deflate(), since the currently available input may have to
				be compressed and flushed. In particular, strm->avail_out must be non-zero.

				deflateParams returns Z_OK if success, Z_STREAM_ERROR if the source
				stream state was inconsistent or if a parameter was invalid, Z_BUF_ERROR
				if strm->avail_out was zero.

********************************************************************************************/

INT32 ZipDeflate::Params(ZStream *strm, INT32 level, INT32 strategy)
{
    DeflateState *s;
    compress_func func;
    INT32 err = Z_OK;

    if (strm == Z_NULL || strm->De_state == Z_NULL)
		return Z_STREAM_ERROR;
    
	s = strm->De_state;

    if (level == Z_DEFAULT_COMPRESSION)
	{
		level = 6;
    }
    
	if (level < 0 || level > 9 || strategy < 0 || strategy > Z_HUFFMAN_ONLY)
	{
		return Z_STREAM_ERROR;
    }
    
	func = configuration_table[s->level].func;

    if (func != configuration_table[level].func && strm->total_in != 0)
	{
		/* Flush the last buffer: */
		err = deflate(strm, Z_PARTIAL_FLUSH);
    }
    if (s->level != level)
	{
		s->level = level;
		s->max_lazy_match   = configuration_table[level].max_lazy;
		s->good_match       = configuration_table[level].good_length;
		s->nice_match       = configuration_table[level].nice_length;
		s->max_chain_length = configuration_table[level].max_chain;
    }

    s->strategy = strategy;
    return err;
}

/********************************************************************************************

>	

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Put a short the pending_out buffer. The 16-bit value is put in MSB order.
				IN assertion: the stream state is correct and there is enough room in
				the pending_out buffer.

********************************************************************************************/

void ZipDeflate::putShortMSB( DeflateState *s, uInt b)
{
    put_byte(s, (Byte)(b >> 8));
    put_byte(s, (Byte)(b & 0xff));
}   

/********************************************************************************************

>	void ZipDeflate::flush_pending(ZStream *strm)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Flush as much pending output as possible. All deflate() output goes
				through this function so some applications may wish to modify it
				to avoid allocating a large strm->next_out buffer and copying into it.
				(See also read_buf()).

********************************************************************************************/

void ZipDeflate::flush_pending(ZStream *strm)
{
    unsigned len = strm->De_state->pending;

    if (len > strm->avail_out) len = strm->avail_out;
    if (len == 0)
		return;

    zmemcpy(strm->next_out, strm->De_state->pending_out, len);
    strm->next_out  += len;
    strm->De_state->pending_out  += len;
    strm->total_out += len;
    strm->avail_out  -= len;
    strm->De_state->pending -= len;
    if (strm->De_state->pending == 0)
    {
        strm->De_state->pending_out = strm->De_state->pending_buf;
    }
}

/********************************************************************************************

>	INT32 ZipDeflate::deflate(ZStream *strm, INT32 flush)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Actually go and deflate the stream.

				Performs one or both of the following actions:

				- Compress more input starting at next_in and update next_in and avail_in
				accordingly. If not all input can be processed (because there is not
				enough room in the output buffer), next_in and avail_in are updated and
				processing will resume at this point for the next call of deflate().

				- Provide more output starting at next_out and update next_out and avail_out
				accordingly. This action is forced if the parameter flush is non zero.
				Forcing flush frequently degrades the compression ratio, so this parameter
				should be set only when necessary (in interactive applications).
				Some output may be provided even if flush is not set.

				Before the call of deflate(), the application should ensure that at least
				one of the actions is possible, by providing more input and/or consuming
				more output, and updating avail_in or avail_out accordingly; avail_out
				should never be zero before the call. The application can consume the
				compressed output when it wants, for example when the output buffer is full
				(avail_out == 0), or after each call of deflate(). If deflate returns Z_OK
				and with zero avail_out, it must be called again after making room in the
				output buffer because there might be more output pending.

				If the parameter flush is set to Z_PARTIAL_FLUSH, the current compression
				block is terminated and flushed to the output buffer so that the
				decompressor can get all input data available so far. For method 9, a future
				variant on method 8, the current block will be flushed but not terminated.
				If flush is set to Z_FULL_FLUSH, the compression block is terminated, a
				special marker is output and the compression dictionary is discarded; this
				is useful to allow the decompressor to synchronize if one compressed block
				has been damaged (see inflateSync below).  Flushing degrades compression and
				so should be used only when necessary.  Using Z_FULL_FLUSH too often can
				seriously degrade the compression.

				If the parameter flush is set to Z_FINISH, all pending input is processed,
				all pending output is flushed and deflate returns with ZStream_END if there
				was enough output space; if deflate returns with Z_OK, this function must be
				called again with Z_FINISH and more output space (updated avail_out) but no
				more input data, until it returns with ZStream_END or an error. After
				deflate has returned ZStream_END, the only possible operations on the
				stream are deflateReset or deflateEnd.

				Z_FINISH can be used immediately after deflateInit if all the compression
				is to be done in a single step. In this case, avail_out must be at least
				0.1% larger than avail_in plus 12 bytes.  If deflate does not return
				ZStream_END, then it must be called again as described above.

				deflate() may update data_type if it can make a good guess about
				the input data type (Z_ASCII or Z_BINARY). In doubt, the data is considered
				binary. This field is only for information purposes and does not affect
				the compression algorithm in any manner.

				deflate() returns Z_OK if some progress has been made (more input
				processed or more output produced), ZStream_END if all input has been
				consumed and all output has been produced (only when flush is set to
				Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
				if next_in or next_out was NULL), Z_BUF_ERROR if no progress is possible.

********************************************************************************************/

INT32 ZipDeflate::deflate(ZStream *strm, INT32 flush)
{
    INT32 old_flush; /* value of flush param for previous deflate call */
    DeflateState *s;

    if (strm == Z_NULL || strm->De_state == Z_NULL || flush > Z_FINISH || flush < 0)
	{
        return Z_STREAM_ERROR;
    }

    s = strm->De_state;
    
    if (strm->next_out == Z_NULL ||
        (strm->next_in == Z_NULL && strm->avail_in != 0) ||
		(s->status == FINISH_STATE && flush != Z_FINISH))
	{
        ERR_RETURN(strm, Z_STREAM_ERROR);
    }

    if (strm->avail_out == 0)
		ERR_RETURN(strm, Z_BUF_ERROR);

    strm->De_state->strm = strm; /* just in case */
    old_flush = s->last_flush;
    s->last_flush = flush;

    /* Write the zlib header */
    if (s->status == INIT_STATE)
	{
        uInt header = (Z_DEFLATED + ((s->w_bits-8)<<4)) << 8;
        uInt level_flags = (s->level-1) >> 1;

        if (level_flags > 3)
			level_flags = 3;
        header |= (level_flags << 6);

		if (s->strstart != 0)
			header |= PRESET_DICT;
        header += 31 - (header % 31);

        s->status = BUSY_STATE;
        putShortMSB(s, header);

		/* Save the adler32 of the preset dictionary: */
		if (s->strstart != 0)
		{
			putShortMSB(s, (uInt)(strm->adler >> 16));
			putShortMSB(s, (uInt)(strm->adler & 0xffff));
		}
		strm->adler = 1L;
    }

    /* Flush as much pending output as possible */
    if (s->pending != 0)
	{
        flush_pending(strm);
        if (strm->avail_out == 0)
		{
			/* Since avail_out is 0, deflate will be called again with
			 * more output space, but possibly with both pending and
			 * avail_in equal to zero. There won't be anything to do,
			 * but this is not an error situation so make sure we
			 * return OK instead of BUF_ERROR at next call of deflate:
				 */
			s->last_flush = -1;
			return Z_OK;
		}

    /* Make sure there is something to do and avoid duplicate consecutive
     * flushes. For repeated and useless calls with Z_FINISH, we keep
     * returning Z_STREAM_END instead of Z_BUFF_ERROR.
     */
    }
	else if (strm->avail_in == 0 && flush <= old_flush && flush != Z_FINISH)
	{
        ERR_RETURN(strm, Z_BUF_ERROR);
    }
    
	/* User must not provide more input after the first FINISH: */
    if (s->status == FINISH_STATE && strm->avail_in != 0)
	{
        ERR_RETURN(strm, Z_BUF_ERROR);
    }

    /* Start a new block or continue the current one.
     */
    if (strm->avail_in != 0 || s->lookahead != 0 ||
        (flush != Z_NO_FLUSH && s->status != FINISH_STATE))
    {
        INT32 quit;
        
        if (flush == Z_FINISH)
        {
            s->status = FINISH_STATE;
        }
	
		//quit = (*(configuration_table[s->level].func))(s, flush);
		switch (configuration_table[s->level].func)
		{
			case Z_deflate_stored : 
				quit = deflate_stored(s, flush);
				break;	
			case Z_deflate_fast : 
				quit = deflate_fast(s, flush);
				break;	
			case Z_deflate_slow : 
				quit = deflate_slow(s, flush);
				break;	
			default:
				// We have a type we don't understand so error
				return Z_STREAM_ERROR;
		}

		if (strm->avail_out == 0)
		{
			s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
		}

        if (quit || strm->avail_out == 0)
			return Z_OK;
        /* If flush != Z_NO_FLUSH && avail_out == 0, the next call
         * of deflate should use the same flush parameter to make sure
         * that the flush is complete. So we don't have to output an
         * empty block here, this will be done at next call. This also
         * ensures that for a very small output buffer, we emit at most
         * one empty block.
         */
		 if (flush != Z_NO_FLUSH && flush != Z_FINISH)
		 {
            if (flush == Z_PARTIAL_FLUSH)
			{
                _tr_align(s);
            }
			else
			{
				/* FULL_FLUSH or SYNC_FLUSH */
                _tr_stored_block(s, (char*)0, 0L, 0);
                /* For a full flush, this empty block will be recognized
                 * as a special marker by inflate_sync().
                 */
                if (flush == Z_FULL_FLUSH)
				{
                    CLEAR_HASH(s);             /* forget history */
                }
            }
            
			flush_pending(strm);
            
			if (strm->avail_out == 0)
			{
			  s->last_flush = -1; /* avoid BUF_ERROR at next call, see above */
			  return Z_OK;
			}
        }
    }
    Assert(strm->avail_out > 0, "bug2");

    if (flush != Z_FINISH)
		return Z_OK;
    if (s->noheader)
		return Z_STREAM_END;

    /* Write the zlib trailer (adler32) */
    putShortMSB(s, (uInt)(strm->adler >> 16));
    putShortMSB(s, (uInt)(strm->adler & 0xffff));
    flush_pending(strm);
    /* If avail_out is zero, the application will call deflate again
     * to flush the rest.
     */
    s->noheader = -1; /* write the trailer only once! */
    return s->pending != 0 ? Z_OK : Z_STREAM_END;
}

/********************************************************************************************

>	INT32 ZipDeflate::End(ZStream *strm)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	End the deflation on this stream.

				All dynamically allocated data structures for this stream are freed.
				This function discards any unprocessed input and does not flush any
				pending output.

				deflate End returns Z_OK if success, Z_STREAM_ERROR if the
				stream state was inconsistent. In the error case, msg may be set
				but then points to a static string (which must not be deallocated).

********************************************************************************************/

INT32 ZipDeflate::End(ZStream *strm)
{
    if (strm == NULL || strm->De_state == NULL) return Z_STREAM_ERROR;

    INT32 status;
	
    /* Deallocate in reverse order of allocations: */
    TRY_FREE(strm, strm->De_state->pending_buf);
    TRY_FREE(strm, strm->De_state->head);
    TRY_FREE(strm, strm->De_state->prev);
    TRY_FREE(strm, strm->De_state->window);

    status = strm->De_state->status;
	delete strm->De_state;
    strm->De_state = NULL;

    return status == BUSY_STATE ? Z_DATA_ERROR : Z_OK;
}

/********************************************************************************************

>	

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	

********************************************************************************************/

//* ========================================================================= */
	/*
	     Sets the destination stream as a complete copy of the source stream.  If
	   the source stream is using an application-supplied history buffer, a new
	   buffer is allocated for the destination stream.  The compressed output
	   buffer is always application-supplied. It's the responsibility of the
	   application to provide the correct values of next_out and avail_out for the
	   next call of deflate.

	     This function is useful when several compression strategies will be
	   tried, for example when there are several ways of pre-processing the input
	   data with a filter. The streams that will be discarded should then be freed
	   by calling deflateEnd.  Note that deflateCopy duplicates the internal
	   compression state which can be quite large, so this strategy is slow and
	   can consume lots of memory.

	      deflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
	   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
	   (such as zalloc being NULL). msg is left unchanged in both source and
	   destination.
	*/

#if 0
INT32 ZipDeflate::Copy(ZStream *dest, ZStream *source)
{
    if (source == Z_NULL || dest == Z_NULL || source->state == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    *dest = *source;
    return Z_STREAM_ERROR; /* to be implemented */
#if 0
    dest->state = (struct internal_state FAR *)
        (*dest->zalloc)(1, sizeof(deflate_state));
    if (dest->state == Z_NULL) return Z_MEM_ERROR;

    *(dest->state) = *(source->state);
    return Z_OK;
#endif
}
#endif


/********************************************************************************************

>	INT32 ZipDeflate::read_buf(ZStream *strm, char *buf, unsigned size)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Read a new buffer from the current input stream, update the adler32
				and total number of bytes read.  All deflate() input goes through
				this function so some applications may wish to modify it to avoid
				allocating a large strm->next_in buffer and copying from it.
				(See also flush_pending()).
********************************************************************************************/

INT32 ZipDeflate::read_buf(ZStream *strm, charf *buf, unsigned size)
{
    unsigned len = strm->avail_in;

    if (len > size) len = size;
    if (len == 0) return 0;

    strm->avail_in  -= len;

    if (!strm->De_state->noheader)
    {
        strm->adler = GZipFile::adler32(strm->adler, strm->next_in, len);
    }
    zmemcpy(buf, strm->next_in, len);
    strm->next_in  += len;
    strm->total_in += len;

    return (INT32)len;
}

/********************************************************************************************

>	void ZipDeflate::lm_init( DeflateState *s)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Initialize the "longest match" routines for a new zlib stream

********************************************************************************************/

void ZipDeflate::lm_init( DeflateState *s)
{
    s->window_size = (ulg)2L*s->w_size;

    CLEAR_HASH(s);

    /* Set the default configuration parameters:
     */
    s->max_lazy_match   = configuration_table[s->level].max_lazy;
    s->good_match       = configuration_table[s->level].good_length;
    s->nice_match       = configuration_table[s->level].nice_length;
    s->max_chain_length = configuration_table[s->level].max_chain;

    s->strstart = 0;
    s->block_start = 0L;
    s->lookahead = 0;
    s->match_length = s->prev_length = MIN_MATCH-1;
    s->match_available = 0;
    s->ins_h = 0;
#ifdef ASMV
    match_init(); /* initialize the asm code */
#endif
}

/********************************************************************************************

>	uInt ZipDeflate::longest_match( DeflateState *s, IPos cur_match)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Set match_start to the longest match starting at the given string and
				return its length. Matches shorter or equal to prev_length are discarded,
				in which case the result is equal to prev_length and match_start is
				garbage.
				IN assertions: cur_match is the head of the hash chain for the current
					string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
				OUT assertion: the match length is not greater than s->lookahead.
				
				For 80x86 and 680x0, an optimized version will be provided in match.asm or
				match.S. The code will be functionally equivalent.

********************************************************************************************/

uInt ZipDeflate::longest_match( DeflateState *s, IPos cur_match)
{
    unsigned chain_length = s->max_chain_length;/* max hash chain length */
    register Bytef *scan = s->window + s->strstart; /* current string */
    register Bytef *match;                       /* matched string */
    register INT32 len;                           /* length of current match */
    INT32 best_len = s->prev_length;              /* best match length so far */
    INT32 nice_match = s->nice_match;             /* stop if match long enough */
    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
        s->strstart - (IPos)MAX_DIST(s) : NIL;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */
    Posf *prev = s->prev;
    uInt wmask = s->w_mask;

#ifdef UNALIGNED_OK
    /* Compare two bytes at a time. Note: this is not always beneficial.
     * Try with and without -DUNALIGNED_OK to check.
     */
    register Bytef *strend = s->window + s->strstart + MAX_MATCH - 1;
    register ush scan_start = *(ushf*)scan;
    register ush scan_end   = *(ushf*)(scan+best_len-1);
#else
    register Bytef *strend = s->window + s->strstart + MAX_MATCH;
    register Byte scan_end1  = scan[best_len-1];
    register Byte scan_end   = scan[best_len];
#endif

    /* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
     * It is easy to get rid of this optimization if necessary.
     */
    Assert(s->hash_bits >= 8 && MAX_MATCH == 258, "Code too clever");

    /* Do not waste too much time if we already have a good match: */
    if (s->prev_length >= s->good_match) {
        chain_length >>= 2;
    }
    /* Do not look for matches beyond the end of the input. This is necessary
     * to make deflate deterministic.
     */
    if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;

    Assert((ulg)s->strstart <= s->window_size-MIN_LOOKAHEAD, "need lookahead");

    do {
        Assert(cur_match < s->strstart, "no future");
        match = s->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2:
         */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
        /* This code assumes sizeof(unsigned short) == 2. Do not use
         * UNALIGNED_OK if your compiler uses a different size.
         */
        if (*(ushf*)(match+best_len-1) != scan_end ||
            *(ushf*)match != scan_start) continue;

        /* It is not necessary to compare scan[2] and match[2] since they are
         * always equal when the other bytes match, given that the hash keys
         * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
         * strstart+3, +5, ... up to strstart+257. We check for insufficient
         * lookahead only every 4th comparison; the 128th check will be made
         * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
         * necessary to put more guard bytes at the end of the window, or
         * to check more often for insufficient lookahead.
         */
        Assert(scan[2] == match[2], "scan[2]?");
        scan++, match++;
        do {
        } while (*(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
                 *(ushf*)(scan+=2) == *(ushf*)(match+=2) &&
                 scan < strend);
        /* The funny "do {}" generates better code on most compilers */

        /* Here, scan <= window+strstart+257 */
        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");
        if (*scan == *match) scan++;

        len = (MAX_MATCH - 1) - (INT32)(strend-scan);
        scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

        if (match[best_len]   != scan_end  ||
            match[best_len-1] != scan_end1 ||
            *match            != *scan     ||
            *++match          != scan[1])      continue;

        /* The check at best_len-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare scan[2] and match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        scan += 2, match++;
        Assert(*scan == *match, "match[2]?");

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at strstart+258.
         */
        do {
        } while (*++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 scan < strend);

        Assert(scan <= s->window+(unsigned)(s->window_size-1), "wild scan");

        len = MAX_MATCH - (INT32)(strend - scan);
        scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

        if (len > best_len) {
            s->match_start = cur_match;
            best_len = len;
            if (len >= nice_match) break;
#ifdef UNALIGNED_OK
            scan_end = *(ushf*)(scan+best_len-1);
#else
            scan_end1  = scan[best_len-1];
            scan_end   = scan[best_len];
#endif
        }
    } while ((cur_match = prev[cur_match & wmask]) > limit
             && --chain_length != 0);

    if ((uInt)best_len <= s->lookahead) return best_len;

    return s->lookahead;
}

/********************************************************************************************

>	void ZipDeflate::check_match( DeflateState *s, IPos start, IPos match, INT32 length)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Check that the match at match_start is indeed a match.

********************************************************************************************/

#ifdef DEBUG

void ZipDeflate::check_match( DeflateState *s, IPos start, IPos match, INT32 length)
{
    /* check that the match is indeed a match */
    if (zmemcmp((charf *)s->window + match,
                (charf *)s->window + start, length) != EQUAL) {
        _ftprintf(stderr, " start %u, match %u, length %d\n",
		start, match, length);
        do {
	    _ftprintf(stderr, "%c%c", s->window[match++], s->window[start++]);
	} while (--length != 0);
        z_error("invalid match");
    }
    if (verbose > 1) {
        _ftprintf(stderr,"\\[%d,%d]", start-match, length);
        do { _puttc(s->window[start++], stderr); } while (--length != 0);
    }
}
#else 
#  define check_match(s, start, match, length)
#endif

/********************************************************************************************

>	

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Fill the window when the lookahead becomes insufficient.
				Updates strstart and lookahead.
				 *
				IN assertion: lookahead < MIN_LOOKAHEAD
				OUT assertions: strstart <= window_size-MIN_LOOKAHEAD
				   At least one byte has been read, or avail_in == 0; reads are
				   performed for at least two bytes (required for the zip translate_eol
				   option -- not supported here).

********************************************************************************************/

void ZipDeflate::fill_window( DeflateState *s)
{
    register unsigned n, m;
    register Posf *p;
    unsigned more;    /* Amount of free space at the end of the window. */
    uInt wsize = s->w_size;

    do {
        more = (unsigned)(s->window_size -(ulg)s->lookahead -(ulg)s->strstart);

        /* Deal with !@#$% 64K limit: */
        if (more == 0 && s->strstart == 0 && s->lookahead == 0) {
            more = wsize;

        } else if (more == (unsigned)(-1)) {
            /* Very unlikely, but possible on 16 bit machine if strstart == 0
             * and lookahead == 1 (input done one byte at time)
             */
            more--;

        /* If the window is almost full and there is insufficient lookahead,
         * move the upper half to the lower one to make room in the upper half.
         */
        } else if (s->strstart >= wsize+MAX_DIST(s)) {

            zmemcpy((charf *)s->window, (charf *)s->window+wsize,
                   (unsigned)wsize);
            s->match_start -= wsize;
            s->strstart    -= wsize; /* we now have strstart >= MAX_DIST */

            s->block_start -= (INT32) wsize;

            /* Slide the hash table (could be avoided with 32 bit values
               at the expense of memory usage):
             */
            n = s->hash_size;
            p = &s->head[n];
            do {
                m = *--p;
                *p = (Pos)(m >= wsize ? m-wsize : NIL);
            } while (--n);

            n = wsize;
            p = &s->prev[n];
            do {
                m = *--p;
                *p = (Pos)(m >= wsize ? m-wsize : NIL);
                /* If n is not on any hash chain, prev[n] is garbage but
                 * its value will never be used.
                 */
            } while (--n);

            more += wsize;
        }
        if (s->strm->avail_in == 0) return;

        /* If there was no sliding:
         *    strstart <= WSIZE+MAX_DIST-1 && lookahead <= MIN_LOOKAHEAD - 1 &&
         *    more == window_size - lookahead - strstart
         * => more >= window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
         * => more >= window_size - 2*WSIZE + 2
         * In the BIG_MEM or MMAP case (not yet supported),
         *   window_size == input_size + MIN_LOOKAHEAD  &&
         *   strstart + s->lookahead <= input_size => more >= MIN_LOOKAHEAD.
         * Otherwise, window_size == 2*WSIZE so more >= 2.
         * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
         */
        Assert(more >= 2, "more < 2");

        n = read_buf(s->strm, (charf *)s->window + s->strstart + s->lookahead,
                     more);
        s->lookahead += n;

        /* Initialize the hash value now that we have some input: */
        if (s->lookahead >= MIN_MATCH) {
            s->ins_h = s->window[s->strstart];
            UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
#if MIN_MATCH != 3
            Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
        }
        /* If the whole input has less than MIN_MATCH bytes, ins_h is garbage,
         * but this is not important since only literal bytes will be emitted.
         */

    } while (s->lookahead < MIN_LOOKAHEAD && s->strm->avail_in != 0);
}

/********************************************************************************************

>	

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Flush the current block, with given end-of-file flag.
				IN assertion: strstart is set to the end of the current match.

********************************************************************************************/

#define FLUSH_BLOCK_ONLY(s, eof) { \
   _tr_flush_block(s, (s->block_start >= 0L ? \
                   (charf *)&s->window[(unsigned)s->block_start] : \
                   (charf *)Z_NULL), \
		(ulg)((INT32)s->strstart - s->block_start), \
		(eof)); \
   s->block_start = s->strstart; \
   flush_pending(s->strm); \
   Tracev((stderr,"[FLUSH]")); \
}

/* Same but force premature exit if necessary. */
#define FLUSH_BLOCK(s, eof) { \
   FLUSH_BLOCK_ONLY(s, eof); \
   if (s->strm->avail_out == 0) return 1; \
}

/********************************************************************************************

>	INT32 ZipDeflate::deflate_fast( DeflateState *s, INT32 flush)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:   	Copy without compression as much as possible from the input stream, return
				true if processing was terminated prematurely (no more input or output
				space).  This function does not insert new strings in the dictionary
				since uncompressible data is probably not useful. This function is used
				only for the level=0 compression option.
				NOTE: this function should be optimized to avoid extra copying.
 
********************************************************************************************/

INT32 ZipDeflate::deflate_stored(DeflateState* s, INT32 flush)
{
    for (;;) {
        /* Fill the window as much as possible: */
        if (s->lookahead <= 1) {

            Assert(s->strstart < s->w_size+MAX_DIST(s) ||
		   s->block_start >= (INT32)s->w_size, "slide too late");

            fill_window(s);
            if (s->lookahead == 0 && flush == Z_NO_FLUSH) return 1;

            if (s->lookahead == 0) break; /* flush the current block */
        }
	Assert(s->block_start >= 0L, "block gone");

	s->strstart += s->lookahead;
	s->lookahead = 0;

        /* Stored blocks are limited to 0xffff bytes: */
        if (s->strstart == 0 || s->strstart > 0xfffe) {
	    /* strstart == 0 is possible when wraparound on 16-bit machine */
	    s->lookahead = s->strstart - 0xffff;
	    s->strstart = 0xffff;
	}

	/* Emit a stored block if it is large enough: */
        if (s->strstart - (uInt)s->block_start >= MAX_DIST(s)) {
            FLUSH_BLOCK(s, 0);
	}
    }
    FLUSH_BLOCK(s, flush == Z_FINISH);
    return 0; /* normal exit */
}

/********************************************************************************************

>	INT32 ZipDeflate::deflate_fast( DeflateState *s, INT32 flush)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:   	Compress as much as possible from the input stream, return true if
				processing was terminated prematurely (no more input or output space).
				This function does not perform lazy evaluationof matches and inserts
				new strings in the dictionary only for unmatched strings or for short
				matches. It is used only for the fast compression options.	

********************************************************************************************/

INT32 ZipDeflate::deflate_fast( DeflateState *s, INT32 flush)
{
    IPos hash_head = NIL; /* head of the hash chain */
    INT32 bflush;           /* set if current block must be flushed */

    for (;;) {
        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) return 1;

            if (s->lookahead == 0) break; /* flush the current block */
        }

        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        if (s->lookahead >= MIN_MATCH) {
            INSERT_STRING(s, s->strstart, hash_head);
        }

        /* Find the longest match, discarding those <= prev_length.
         * At this point we have always match_length < MIN_MATCH
         */
        if (hash_head != NIL && s->strstart - hash_head <= MAX_DIST(s)) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            if (s->strategy != Z_HUFFMAN_ONLY) {
                s->match_length = longest_match (s, hash_head);
            }
            /* longest_match() sets match_start */
        }
        if (s->match_length >= MIN_MATCH) {
            check_match(s, s->strstart, s->match_start, s->match_length);

            bflush = _tr_tally(s, s->strstart - s->match_start,
                               s->match_length - MIN_MATCH);

            s->lookahead -= s->match_length;

            /* Insert new strings in the hash table only if the match length
             * is not too large. This saves time but degrades compression.
             */
            if (s->match_length <= s->max_insert_length &&
                s->lookahead >= MIN_MATCH) {
                s->match_length--; /* string at strstart already in hash table */
                do {
                    s->strstart++;
                    INSERT_STRING(s, s->strstart, hash_head);
                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead.
                     */
                } while (--s->match_length != 0);
                s->strstart++; 
            } else {
                s->strstart += s->match_length;
                s->match_length = 0;
                s->ins_h = s->window[s->strstart];
                UPDATE_HASH(s, s->ins_h, s->window[s->strstart+1]);
#if MIN_MATCH != 3
                Call UPDATE_HASH() MIN_MATCH-3 more times
#endif
                /* If lookahead < MIN_MATCH, ins_h is garbage, but it does not
                 * matter since it will be recomputed at next deflate call.
                 */
            }
        } else {
            /* No match, output a literal byte */
            Tracevv((stderr,"%c", s->window[s->strstart]));
            bflush = _tr_tally (s, 0, s->window[s->strstart]);
            s->lookahead--;
            s->strstart++; 
        }
        if (bflush) FLUSH_BLOCK(s, 0);
    }
    FLUSH_BLOCK(s, flush == Z_FINISH);
    return 0; /* normal exit */
}

/********************************************************************************************

>	INT32 ZipDeflate::deflate_slow( DeflateState *s, INT32 flush)

	Author:		Neville_Humphrys (Xara Group Ltd) <camelotdev@xara.com>
	Created:	24/05/95
	Purpose:	Same as above, but achieves better compression. We use a lazy
				evaluation for matches: a match is finally adopted only if there is
				no better match at the next window position.

********************************************************************************************/

INT32 ZipDeflate::deflate_slow( DeflateState *s, INT32 flush)
{
    IPos hash_head = NIL;    /* head of hash chain */
    INT32 bflush;              /* set if current block must be flushed */

    /* Process the input block. */
    for (;;) {
        /* Make sure that we always have enough lookahead, except
         * at the end of the input file. We need MAX_MATCH bytes
         * for the next match, plus MIN_MATCH bytes to insert the
         * string following the next match.
         */
        if (s->lookahead < MIN_LOOKAHEAD) {
            fill_window(s);
            if (s->lookahead < MIN_LOOKAHEAD && flush == Z_NO_FLUSH) return 1;

            if (s->lookahead == 0) break; /* flush the current block */
        }

        /* Insert the string window[strstart .. strstart+2] in the
         * dictionary, and set hash_head to the head of the hash chain:
         */
        if (s->lookahead >= MIN_MATCH) {
            INSERT_STRING(s, s->strstart, hash_head);
        }

        /* Find the longest match, discarding those <= prev_length.
         */
        s->prev_length = s->match_length, s->prev_match = s->match_start;
        s->match_length = MIN_MATCH-1;

        if (hash_head != NIL && s->prev_length < s->max_lazy_match &&
            s->strstart - hash_head <= MAX_DIST(s)) {
            /* To simplify the code, we prevent matches with the string
             * of window index 0 (in particular we have to avoid a match
             * of the string with itself at the start of the input file).
             */
            if (s->strategy != Z_HUFFMAN_ONLY) {
                s->match_length = longest_match (s, hash_head);
            }
            /* longest_match() sets match_start */

            if (s->match_length <= 5 && (s->strategy == Z_FILTERED ||
                 (s->match_length == MIN_MATCH &&
                  s->strstart - s->match_start > TOO_FAR))) {

                /* If prev_match is also MIN_MATCH, match_start is garbage
                 * but we will ignore the current match anyway.
                 */
                s->match_length = MIN_MATCH-1;
            }
        }
        /* If there was a match at the previous step and the current
         * match is not better, output the previous match:
         */
        if (s->prev_length >= MIN_MATCH && s->match_length <= s->prev_length) {
            uInt max_insert = s->strstart + s->lookahead - MIN_MATCH;
            /* Do not insert strings in hash table beyond this. */

            check_match(s, s->strstart-1, s->prev_match, s->prev_length);

            bflush = _tr_tally(s, s->strstart -1 - s->prev_match,
                               s->prev_length - MIN_MATCH);

            /* Insert in hash table all strings up to the end of the match.
             * strstart-1 and strstart are already inserted. If there is not
             * enough lookahead, the last two strings are not inserted in
             * the hash table.
             */
            s->lookahead -= s->prev_length-1;
            s->prev_length -= 2;
            do {
                if (++s->strstart <= max_insert) {
                    INSERT_STRING(s, s->strstart, hash_head);
                }
            } while (--s->prev_length != 0);
            s->match_available = 0;
            s->match_length = MIN_MATCH-1;
            s->strstart++;

            if (bflush) FLUSH_BLOCK(s, 0);

        } else if (s->match_available) {
            /* If there was no match at the previous position, output a
             * single literal. If there was a match but the current match
             * is longer, truncate the previous match to a single literal.
             */
            Tracevv((stderr,"%c", s->window[s->strstart-1]));
            if (_tr_tally (s, 0, s->window[s->strstart-1])) {
                FLUSH_BLOCK_ONLY(s, 0);
            }
            s->strstart++;
            s->lookahead--;
            if (s->strm->avail_out == 0) return 1;
        } else {
            /* There is no previous match to compare with, wait for
             * the next step to decide.
             */
            s->match_available = 1;
            s->strstart++;
            s->lookahead--;
        }
    }
    Assert (flush != Z_NO_FLUSH, "no flush?");
    if (s->match_available) {
        Tracevv((stderr,"%c", s->window[s->strstart-1]));
        _tr_tally (s, 0, s->window[s->strstart-1]);
        s->match_available = 0;
    }
    FLUSH_BLOCK(s, flush == Z_FINISH);
    return 0;
}
