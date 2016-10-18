// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 - 2008 by Eric Chung, Michael Ferdman, Brian Gold, Nikos   
// Hardavellas, Jangwoo Kim, Ippokratis Pandis, Minglong Shao, Jared Smolens,
// Stephen Somogyi, Evangelos Vlachos, Tom Wenisch, Anastassia Ailamaki,     
// Babak Falsafi and James C. Hoe for the SimFlex Project, Computer          
// Architecture Lab at Carnegie Mellon, Carnegie Mellon University.          
//                                                                           
// For more information, see the SimFlex project website at:                 
//   http://www.ece.cmu.edu/~simflex                                         
//                                                                           
// You may not use the name 'Carnegie Mellon University' or derivations      
// thereof to endorse or promote products derived from this software.        
//                                                                           
// If you modify the software you must place a notice on or within any       
// modified version provided or made available to any third party stating    
// that you have modified the software.  The notice shall include at least   
// your name, address, phone number, email address and the date and purpose  
// of the modification.                                                      
//                                                                           
// THE SOFTWARE IS PROVIDED 'AS-IS' WITHOUT ANY WARRANTY OF ANY KIND, EITHER 
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY  
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY 
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,  
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY 
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT, 
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN   
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY, 
// CONTRACT, TORT OR OTHERWISE).                                             
//                                     
// DO-NOT-REMOVE end-copyright-block   
/*  Filename:    histogram.cpp
 *  Description: 
 * 
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
#include "histogram.h"


/************************************
 * Function name: Histogram::Histogram()
 * Description:   Creates a Histogram object and validates the arguments
 * Arguments:
 *   int *bucketList - a sorted ascending (non-constant) array
 *   of minimum bucket values (the last index is the maximum value for the 
 *   final bucket).
 *   int _bucketCount - the number of elements in the bucketList array
 * Return value:  a new Histogram class
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
Histogram::Histogram ( const COUNTER * bucketList, 
		       const COUNTER   _bucketCount,
		       const bool dontInitialize ) 
{

  /* Do basic consistency checks on the histogram buckets */
  if ( bucketList == NULL ) {
	exit ( 1 );
  }
  
  if ( _bucketCount <= 1 ) {
	exit ( 1 );
  }
  
  bucketCount     = _bucketCount;
  buckets         = new COUNTER[(int)bucketCount];
  bucketValues    = new COUNTER[(int)bucketCount];


  /* Copy in the buckets array and ensure that the values are 
   *  monotonically increasing and not constant
   */
  if ( !dontInitialize ) { 
    initializeBucketList ( bucketList );
    
    clearBuckets();
  }

}


/************************************
 * Function name: Histogram::~Histogram
 * Description:   Destructor for the Histogram class, frees memory
 * Arguments:     none
 * Return value:  none
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
Histogram::~Histogram() 
{
  
  if ( buckets != NULL ) {
	delete[] buckets;
	buckets = NULL;
  }

  if ( bucketValues != NULL ) { 
	delete[] bucketValues;
	bucketValues = NULL;
  }

}



/************************************
 * Function name: Histogram::increment
 * Description:   Increments the appropriate bucket value for the given
 *  datapoint.  
 * Arguments:     const int dataPoint - the dataValue which should fall
 *  under one of the bucket positions
 * Return value:  bool - false if there was a bucket position to hold
 *  the value.  True if no such bucket exists
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::increment ( const COUNTER dataPoint ) 
{

  int pos;

  pos = findBucketPosition ( dataPoint );

  if ( pos == NO_BUCKET ) {
	return true;
  }
  
  bucketValues[pos]++;
  accumulation += dataPoint;

  return false;

}

/************************************
 * Function name: Histogram::findBucketPosition
 * Description:   Returns the appopriate bucket position for the 
 *  given datapoint, if any.
 * Arguments:     const int dataPoint - the point to search for
 * Return value:  int - the bucket position.  If NO_BUCKET is returned,
 *  there was no bucket position which can hold this value
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
int Histogram::findBucketPosition ( const COUNTER dataPoint )
{
  int
	mid,
	min,
	max;

  /* Check the boundaries first */
  if ( dataPoint < buckets[0] ||
	   dataPoint > buckets[bucketCount-1] ) 
	return NO_BUCKET;

  /* Otherwise, the point must lie somewhere inside the bucket range.
   *  Since a large number of buckets may be used, we will try a binary
   *  search.
   */
  mid = (int)bucketCount / 2;
  min = 0;
  max = (int)bucketCount - 1;
  
  while ( 1 ) { 
	if ( dataPoint >= buckets[mid] && 
		 dataPoint <  buckets[mid+1] ) { 
	  return mid;
	} else if ( dataPoint < buckets[mid] ) {
	  max = mid;
	  mid = mid - ( max - min + 1 ) / 2;
	} else { 
	  min = mid;
	  mid = mid + ( max - min + 1 ) / 2;
	}
  }
  
}

/************************************
 * Function name: Histogram::add
 * Description:   Adds a given value to the appropriate bucket for the given
 *  datapoint.  
 * Arguments:     const int dataPoint - the dataValue which should fall
 *  under one of the bucket positions
 *                const int value - the increment value (positive or negative)
 * Return value:  bool - false if there was a bucket position to hold
 *  the value.  True if no such bucket exists.
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::add ( const COUNTER dataPoint,
		      const COUNTER value ) 
{

  int pos;

  pos = findBucketPosition ( dataPoint );

  if ( pos == NO_BUCKET ) {
	return true;
  }
  
  bucketValues[pos] = bucketValues[pos] + value;
  accumulation += dataPoint * value;

  return false;

}

/************************************
 * Function name: Histogram::getBucketMinValue
 * Description:   Returns the minimum datapoint value that will fall within
 *  this bucket 
 * Arguments:     const int  bucket - the bucket index
 *                int       &minValue - the variable to be set by this function
 * Return value:  bool - false on no error, true if the bucket index is out
 *  of range
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::getBucketMinValue ( const COUNTER  bucket,
				    COUNTER       &minValue )
{

  if ( bucket < 0 || 
       bucket >= bucketCount-1 ) 
    return true;
  
  minValue = buckets[bucket];
  
  return false;

}


/************************************
 * Function name: Histogram::getBucketMaxValue
 * Description:   Returns the maxmimum datapoint value that will fall within
 *  this bucket 
 * Arguments:     const int  bucket - the bucket index
 *                int       &maxValue - the variable to be set by this function
 * Return value:  bool - false on no error, true if the bucket index is out
 *  of range
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::getBucketMaxValue ( const COUNTER  bucket,
				    COUNTER       &maxValue )
{

  if ( bucket < 0 || 
	   bucket >= bucketCount-1 ) 
	return true;

  maxValue = buckets[bucket+1];

  return false;

}


/************************************
 * Function name: Histogram::clearBuckets
 * Description:   Zeros all of the counts for the buckets
 * Arguments:     none
 * Return value:  false on no error
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::clearBuckets ( void ) 
{

  for ( int i = 0; i < bucketCount; i++ )
	bucketValues[i] = 0;
  
  accumulation = 0;
  
  return false;
}


/************************************
 * Function name: Histogram::getBucketValue
 * Description:   Returns the value/count in the requested bucket number
 * Arguments:     const int  bucket - the bucket number
 *                int       &value  - the value contained within that bucket 
 * Return value:  bool - false on no error
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::getBucketValue ( const int  bucket,
				 COUNTER       &value )
{
  if ( bucket < 0 ||
       bucket >= bucketCount ) 
    return true;
  
  value = bucketValues[bucket];

  return false;
}

/************************************
 * Function name: Histogram::getNumBuckets
 * Description:   Returns the number of buckets 
 * Arguments:     int &num - the variable which will hold the number of buckets
 * Return value:  bool - false on no error
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::getNumBuckets ( int &num ) 
{
  num = (int)bucketCount - 1;
  return false;
}

/************************************
 * Function name: Histogram::outputCSVStats
 * Description:   output all of the histogram statistics in a simple
 *  CSV style output
 * Arguments:     ostream & out - the output stream
 * Return value:  bool - false on no error
 */
bool Histogram::outputCSVStats ( ostream & out )
{
  int
    i;

  for ( i = 0; i < bucketCount; i++ ) 
    out << bucketValues[i] << COMMA;
    
  return false;
}

/************************************
 * Function name: Histogram::outputCSVFractionalStats
 * Description:   output all of the histogram statistics in a fractional
 *  format where the buckets all sum to 1.0
 * Arguments:     ostream & out - the output stream
 * Return value:  bool - false on no error
 */
bool Histogram::outputCSVFractionalStats ( ostream & out )
{
  int
    i;
  
  double 
    sum = 0.0;

  for ( i = 0; i < bucketCount; i++ ) 
    sum += bucketValues[i];

  if ( sum > 0.0 ) { 

    for ( i = 0; i < bucketCount; i++ ) 
      out << (double)bucketValues[i] / sum << COMMA;

  } else { 

    for ( i = 0; i < bucketCount; i++ )
      out << "0.0" << COMMA;
  
  }     
    
  return false;
}

/************************************
 * Function name: Histogram::outputCSVStats
 * Description:   output all of the histogram statistics in a simple
 *  CSV style output
 * Arguments:     ostream & out - the output stream
 * Return value:  bool - false on no error
 */
bool Histogram::outputCSVHeader ( ostream & out )
{
  for ( int i = 0; i < bucketCount; i++ ) 
    out << buckets[i] << COMMA;

  return false;
}

/************************************
 * Function name: Histogram::initializeBucketList
 * Description:   Copy in the bucketlist and check it for valid ordering
 * Arguments:     const int * bucketList - the list of new bucket boundaries
 * Return value:  bool - false on no error
 */
bool Histogram::initializeBucketList ( const COUNTER * bucketList )
{
  int
    i;

  buckets[0] = bucketList[0];

  for ( i = 1; i < bucketCount; i++ ) { 
    buckets[i] = bucketList[i];
    
    if ( buckets[i-1] >= buckets[i] ) {
      return true;
    }
  }
  
  return false;
}


/************************************
 * Function name: Histogram::getAccumulation
 * Description:   Returns the value of the histogram's accumulation
 *  counter, which adds all of the ( dataPoint * numInstances ) 
 *  values together.
 * Arguments:     int & accum - the accumulator output value
 * Return value:  bool
 */
bool Histogram::getAccumulation ( COUNTER & accum )
{
  accum = accumulation;
  return false;
}

/************************************
 * Function name: Histogram::getPointCount
 * Description:   Returns the number of data points that have been
 * added to the histogram (basically the sum of all of the buckets.
 * Arguments:     int & ptCount - return value of the count
 * Return value:  bool
 */
bool Histogram::getPointCount ( COUNTER & ptCount,
				const bool onlyAboveZero )
{
  int
    i;
  
  ptCount = 0;

  i = 0;
  if ( onlyAboveZero ) {
    while ( buckets[i] <= 0 ) 
      i++;
  }

  for ( ; i < bucketCount; i++ )
    ptCount += bucketValues[i];

  return false;
}
/************************************
 * Function name: Histogram::getMean
 * Description:   Returns the arithmetic mean of the values in the 
 *  histogram
 * Arguments:     int & mean - the mean
 *                const bool onlyAboveZero - only count values above zero,
 *  assumes accumulation/increment value has never been negative
 * Return value:  bool
 */
bool Histogram::getMean ( double & mean,
			  const bool onlyAboveZero )
{
  COUNTER
    ptCount;

  getPointCount ( ptCount, onlyAboveZero );

  if ( ptCount == 0 ) 
    mean = 0;
  else
    mean = (double)accumulation / (double)ptCount;

  return false;
}

/************************************
 * Function name: CategoryHistogram::CategoryHistogram
 * Description:   Category histograms don't care about ordering in the * 
 *  bucket list.  All buckets are of size one.
 * Arguments:     
 * Return value:  bool
 */
CategoryHistogram::CategoryHistogram ( const COUNTER * bucketList,
				       const COUNTER   bucketCount_ )
  : Histogram ( bucketList,
		bucketCount_,
		true )
{
  initializeBucketList ( bucketList );
  clearBuckets();
}

/************************************
 * Function name: CategoryHistogram::CategoryHistogram
 * Description:   Category histograms don't care about ordering in the * 
 *  bucket list.  All buckets are of size one.
 * Arguments:     
 * Return value:  bool
 */
int CategoryHistogram::findBucketPosition ( const COUNTER dataPoint )
{
  int 
    i;

  for ( i = 0; i < bucketCount; i++ )  
    if ( buckets[i] == dataPoint ) 
      return i;

  return NO_BUCKET;
}
/************************************
 * Function name: CategoryHistogram::initalizeBucketList
 * Description:   The category histogram doesn't care about the order
 *  of buckets.
 * Arguments:     const COUNTER * bucketList - the new list of categories 
 *  to carry in
 * Return value:  bool - false on no error
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool CategoryHistogram::initializeBucketList ( const COUNTER * bucketList )
{
  int
    i;

  for ( i = 0; i < bucketCount; i++ )
    buckets[i] = bucketList[i];
     
  return false;
}


/************************************
 * Function name: Histogram::
 * Description:   
 * Arguments:     
 * Return value:  bool
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
CategoryHistogram::~CategoryHistogram ( void )
{
  /* Nothing to do */
}

/************************************
 * Function name: Histogram::
 * Description:   
 * Arguments:     
 * Return value:  bool
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::buildLinearBucketList ( const COUNTER   interval,
                                        const COUNTER   low,
                                        const COUNTER   high,
                                        COUNTER       *& buckets,
                                        COUNTER        & bucketCount )
{
  int
    i;

  if ( low >= high || interval <= 0 ) { 
    return true;
  }

  bucketCount = ( high - low ) / interval + 2;
  
  buckets = new COUNTER[bucketCount];

  for ( i = 0; i < (bucketCount-1); i++ )
    buckets[i] = low + i * interval;

  buckets[bucketCount-1] = HISTOGRAM_POSITIVE_INFINITY;
  
  return false;
}

/************************************
 * Function name: Histogram::
 * Description:   
 * Arguments:     
 * Return value:  bool
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */
bool Histogram::buildLogBucketList ( const double theBase,
                                     const COUNTER low,
                                     const COUNTER high,
                                     COUNTER *& buckets,
                                     COUNTER & bucketCount )
{
  int
    i;

  if ( low >= high || theBase <= 0.0 ) { 
    return true;
  }

  bucketCount = (COUNTER) (logf ( ( (float)( high - low ) ) ) / logf ( (float)theBase ) ) + 2;

  buckets = new COUNTER[bucketCount];

  buckets[0] = low;
  for ( i = 1; i < (bucketCount-1); i++ ) {
    buckets[i] = (COUNTER)(low + pow ( theBase, (double)i ));
  }

  buckets[bucketCount-1] = HISTOGRAM_POSITIVE_INFINITY;
  
  return false;
}

/************************************
 * Function name: Histogram::
 * Description:   
 * Arguments:     
 * Return value:  bool
 * ChangeLog:
 *   Author          Date      Description
 *   --------------- --------  ----------------------------------------
 */




const COUNTER HIST_BUCKETS_POWERS_OF_2[] = 
{
  0, 
  1,
  8,
  64,
  512,
  2048,
  16384,
  131072,
  2097152,
  HISTOGRAM_POSITIVE_INFINITY
};

const int HIST_BUCKETS_POWERS_OF_2_COUNT = 10;

