/*
© [2013] LinkedIn Corp. All rights reserved.
Licensed under the Apache License, Version 2.0 (the "License"); you may
not use this file except in compliance with the License. You may obtain
a copy of the License at  http://www.apache.org/licenses/LICENSE-2.0
 
Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS"
BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.
*/
#include "qtypes.h"
// START FUNC DECL
void
is_sorted_alldef_I2(
    short *iX,
    long long nR,
    SORT_TYPE *ptr_is_sorted
    )
// STOP FUNC DECL
{
  short first_val, second_val, prev_val, curr_val;
  SORT_TYPE sort_order;
  bool second_val_seen = false;
  long long i = 0;
  if ( nR == 0 ) { *ptr_is_sorted = unknown; return; } /* undefined */
  if ( nR == 1 ) { *ptr_is_sorted = ascending; return; } /* ascending */
  first_val  = iX[0];
  for ( i = 1; i < nR; i++ ) {
    if ( iX[i] != first_val ) { 
      second_val = iX[i];
      second_val_seen = true;
      break;
    }
  }
  if ( !second_val_seen ) {  *ptr_is_sorted = ascending; return; } /* ascending */
  if ( first_val < second_val ) { 
    sort_order = ascending;
  }
  else {
    sort_order = descending;
  }
  if ( i == nR ) { return; } // Nothing more to process
  prev_val = second_val;
  iX += i; /* since we have consumed first 2 values */
  nR -= i; /* since we have consumed first 2 values */
  for ( ; i < nR; i++ ) { 
    curr_val = iX[i];
    switch ( sort_order ) { 
      case ascending : 
	if ( curr_val < prev_val ) { *ptr_is_sorted = unsorted; return; } break;
	if ( curr_val > prev_val ) { prev_val = curr_val; }
	break;
      case descending : 
	if ( curr_val > prev_val ) { *ptr_is_sorted = unsorted; return; } break;
	if ( curr_val < prev_val ) { prev_val = curr_val; }
      default :
	exit(-1);
    }
    prev_val = curr_val;
  }
  *ptr_is_sorted = sort_order;
}
