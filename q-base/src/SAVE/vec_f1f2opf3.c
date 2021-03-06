#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "constants.h"
#include "macros.h"
#include "sqlite3.h"
#include "qtypes.h"
#include "fsize.h"
#include "auxil.h"
#include "open_temp_file.h"
#include "dbauxil.h"
#include "aux_fld_meta.h"
#include "vec_f1f2opf3.h"
#include "get_sz_type_op_fld.h"
#include "mk_file.h"
#include "chunking.h"

#include "add2_int.h"
#include "sub2_int.h"
#include "mul2_int.h"
#include "div2_int.h"
#include "rem2_int.h"
#include "shiftleft2_int.h"
#include "shiftright2_int.h"

#include "add2_longlong.h"
#include "sub2_longlong.h"
#include "mul2_longlong.h"
#include "div2_longlong.h"
#include "rem2_longlong.h"
#include "shiftleft2_longlong.h"
#include "shiftright2_longlong.h"

#include "add2_float.h"
#include "sub2_float.h"
#include "mul2_float.h"
#include "div2_float.h"

#include "and2_bool.h"
#include "or2_bool.h"

#include "bwise_and2_int.h"
#include "bwise_and2_longlong.h"
#include "bwise_and2_bool.h"

#include "bwise_or2_int.h"
#include "bwise_or2_longlong.h"
#include "bwise_or2_bool.h"

#include "bwise_xor2_int.h"
#include "bwise_xor2_longlong.h"
#include "bwise_xor2_bool.h"

#include "cmp_ge_int.h"
#include "cmp_le_int.h"
#include "cmp_eq_int.h"
#include "cmp_ne_int.h"
#include "cmp_gt_int.h"
#include "cmp_lt_int.h"

#include "cmp_ge_longlong.h"
#include "cmp_le_longlong.h"
#include "cmp_eq_longlong.h"
#include "cmp_ne_longlong.h"
#include "cmp_gt_longlong.h"
#include "cmp_lt_longlong.h"

#include "assign_char.h"

#include "concat_int_int_to_longlong.h"

static int nT; /* num_threads */
static long long nR; /* number of rows to process */
static int nC = CACHE_SIZE; /* number of rows to process at a time (cache aware) */
static char *op_X = NULL; size_t n_op_X; /* for mmaped output file */
static char *nn_op_X = NULL; size_t n_nn_op_X; /* for mmaped nn file */
static int   f1type;
static int   f2type;
static char *f1_X;
static char *f2_X;
static char *nn_f1_X;
static char *nn_f2_X;
static char *op;
static bool is_nn_field_needed;

void
nn_core(
    char *g1_X, 
    char *g2_X, 
    long long nX, 
    char *g_op_X
    )
{
  for ( long long i = 0; i < nX; i++ ) { 
    *g_op_X++ = *g1_X++ & *g2_X++;
  }
}


void *f1f2opf3_core(
		    void *threadid
		    )
{
  int status = 0;
  long tid;
  char *g_op_X = NULL, *g_nn_op_X = NULL;
  char *g1_X, *g2_X = NULL, *g1_nn_X = NULL, *g2_nn_X = NULL;
  long long lb, lb1, lb2, ub, ub1, ub2;
  int num_rows_this_thread; /* number of rows for this thread to process */
  long long nB; /* number of blocks to process in inner loop */
  int nX; /* number of rows handed to vector code */

  tid = (long)threadid;
  // printf("Hello World! It's me, thread #%ld!\n", tid);
  for ( int t = 0; t < nT; t++ ) {
    if ( t != tid ) { continue; }
    status = get_outer_lb_ub(nR, nT, t, &lb1, &ub1);
    cBYE(status);
    num_rows_this_thread = ub1 - lb1;
    status = get_num_blocks( num_rows_this_thread, nC, &nB);
    cBYE(status);
    for ( long long b = 0; b < nB; b++ ) {
      lb2 = b * nC;
      ub2 = lb2 + nC;
      if ( ub2 > num_rows_this_thread ) { ub2 = num_rows_this_thread; }
      cBYE(status);
      lb = lb1 + lb2;
      ub = lb1 + ub2;
      nX = ub2 - lb2;
      /*
	fprintf(stderr, "Thread %ld processing %lld rows from [%lld, %lld] \n", 
	tid, (ub - lb), lb, ub);
      */
      if ( is_nn_field_needed ) {
        g1_nn_X = (nn_f1_X + ( sizeof(char) * lb ));
        g2_nn_X = (nn_f2_X + ( sizeof(char) * lb ));
        g_nn_op_X = (nn_op_X + ( sizeof(char) * lb ));
        nn_core(g1_nn_X, g2_nn_X, nX, g_nn_op_X);
      }
      
      if ( f1type == FLDTYPE_INT ) {
        g1_X = (f1_X + ( sizeof(int) * lb ));
        g2_X = (f2_X + ( sizeof(int) * lb ));
	if ( strcmp(op, "+") == 0 ) {
	  g_op_X = (op_X + ( sizeof(int) * lb ));
	  add2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "-") == 0 ) {
          g_op_X = (op_X + ( sizeof(int) * lb ));
	  sub2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "*") == 0 ) {
          g_op_X = (op_X + ( sizeof(int) * lb ));
	  mul2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "/") == 0 ) {
          g_op_X = (op_X + ( sizeof(int) * lb ));
	  div2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "%") == 0 ) {
          g_op_X = (op_X + ( sizeof(int) * lb ));
	  rem2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "concat") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  concat_int_int_to_long((int *)g1_X, (int *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, ">=") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_ge_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "<=") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_le_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "==") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_eq_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "!=") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_ne_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, ">") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_gt_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "<") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_lt_int((int *)g1_X, (int *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "&") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  bwise_and2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "|") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  bwise_or2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "^") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  bwise_xor2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, "<<") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  shiftleft2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else if ( strcmp(op, ">>") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  shiftright2_int((int *)g1_X, (int *)g2_X, nX, (int *)g_op_X);
	}
	else { go_BYE(-1); }
      }
      else if (f1type == FLDTYPE_LONGLONG ) {
        g1_X = (f1_X + ( sizeof(long long) * lb ));
        g2_X = (f2_X + ( sizeof(long long) * lb ));
        g_op_X = (op_X + ( sizeof(long long) * lb ));
	if ( strcmp(op, "+") == 0 ) {
	  add2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "-") == 0 ) {
	  sub2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "*") == 0 ) {
	  mul2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "/") == 0 ) {
	  div2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "%") == 0 ) {
	  rem2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, ">=") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  cmp_ge_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "<=") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  cmp_le_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "==") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  cmp_eq_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "!=") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  cmp_ne_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, ">") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  cmp_gt_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "<") == 0 ) {
          g_op_X = (op_X + ( sizeof(char) * lb ));
	  cmp_lt_longlong((long long *)g1_X, (long long *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "&") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  bwise_and2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "|") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  bwise_or2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "^") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  bwise_xor2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, "<<") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  shiftleft2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else if ( strcmp(op, ">>") == 0 ) {
          g_op_X = (op_X + ( sizeof(long long) * lb ));
	  shiftright2_longlong((long long *)g1_X, (long long *)g2_X, nX, (long long *)g_op_X);
	}
	else { 
	  fprintf(stderr, "Unknown up = [%s] \n", op);
	  go_BYE(-1); 
	}
      }
      else if ( f1type == FLDTYPE_FLOAT ) {
        g1_X = (f1_X + ( sizeof(float) * lb ));
        g2_X = (f2_X + ( sizeof(float) * lb ));
        g_op_X = (op_X + ( sizeof(float) * lb ));
	if ( strcmp(op, "+") == 0 ) {
	  add2_float((float *)g1_X, (float *)g2_X, nX, (float *)g_op_X);
	}
	else if ( strcmp(op, "-") == 0 ) {
	  sub2_float((float *)g1_X, (float *)g2_X, nX, (float *)g_op_X);
	}
	else if ( strcmp(op, "*") == 0 ) {
	  mul2_float((float *)g1_X, (float *)g2_X, nX, (float *)g_op_X);
	}
	else if ( strcmp(op, "/") == 0 ) {
	  div2_float((float *)g1_X, (float *)g2_X, nX, (float *)g_op_X);
	}
	else { go_BYE(-1); }
      }
      else if ( f1type == FLDTYPE_BOOL ) {
        g1_X = (f1_X + ( sizeof(char) * lb ));
        g2_X = (f2_X + ( sizeof(char) * lb ));
        g_op_X = (op_X + ( sizeof(char) * lb ));
	if ( strcmp(op, "||") == 0 ) {
	  or2_bool((char *)g1_X, (char *)g2_X, nX, (char *)g_op_X);
	}
	else if ( strcmp(op, "&&") == 0 ) {
	  and2_bool((char *)g1_X, (char *)g2_X, nX, (char *)g_op_X);
	}
	else { go_BYE(-1); }
      }
    }
  }
 BYE:
  if ( nT == 1 ) {
    if ( status == 0 ) {
      return ((void *)0);
    }
    else {
      return ((void *)1);
    }
  }
  pthread_exit(NULL);
}


//---------------------------------------------------------------
// START FUNC DECL 
int 
vec_f1f2opf3(
	     long long in_nR,
	     int in_f1type,
	     int in_f2type,
	     char *str_meta_data,
	     char *in_f1_X,
	     char *in_nn_f1_X,
	     char *in_f2_X,
	     char *in_nn_f2_X,
	     char *in_op,
	     char **ptr_opfile,
	     char **ptr_nn_opfile
	     )
// STOP FUNC DECL 
{
  int status = 0;
  FILE *ofp = NULL; char *opfile = NULL;
  FILE *nn_ofp = NULL; char *nn_opfile = NULL;
  int sizeof_op; /* sizeof each cell of output */
  int rc; // result code for thread create 
  pthread_t threads[MAX_NUM_THREADS];
  pthread_attr_t attr;
  void *thread_status;
  FLD_META_TYPE out_fld_meta; int optype;
  
  nR = in_nR;
  f1type = in_f1type;
  f2type = in_f2type;
  f1_X   = in_f1_X;
  f2_X   = in_f2_X;
  nn_f1_X   = in_nn_f1_X;
  nn_f2_X   = in_nn_f2_X;
  op     = in_op;

  if ( f1type != f2type ) { 
    fprintf(stderr, "f1 and f2 are not same type \n"); go_BYE(-1);
  }
  //--------------------------------------------
  // create an output file and mmap it 
  status = open_temp_file(&ofp, &opfile);
  cBYE(status);
  fclose_if_non_null(ofp);
  status = get_sz_type_op_fld("f1f2opf3", op, f1type, f2type, "", 
      &optype, &sizeof_op);
  cBYE(status);
  //------------------------------------------
  zero_fld_meta(&out_fld_meta);
  strcpy(out_fld_meta.filename, opfile);
  status = mk_str_fldtype(optype, out_fld_meta.fldtype);
  cBYE(status);
  out_fld_meta.n_sizeof = sizeof_op;
  status = fld_meta_to_str(out_fld_meta, str_meta_data);
  cBYE(status);

  //------------------------------------------
  n_op_X = sizeof_op * nR;
  status = mk_file(opfile, n_op_X);
  cBYE(status);
  status = rs_mmap(opfile, &op_X, &n_op_X, 1);
  cBYE(status);
  //--------------------------------------------
  status = get_num_threads(&nT);
  cBYE(status);
  //--------------------------------------------
  is_nn_field_needed = false;
  if ( ( in_nn_f1_X == NULL ) && ( in_nn_f2_X == NULL ) ) {
    /* No need for nn file */
  }
  else if ( ( in_nn_f1_X == NULL ) && ( in_nn_f2_X != NULL ) ) {
    status = open_temp_file(&nn_ofp, &nn_opfile);
    cBYE(status);
    fclose_if_non_null(nn_ofp);
    status = mk_file(nn_opfile, sizeof(char) * nR);
    cBYE(status);
    rs_mmap(nn_opfile, &nn_op_X, &n_nn_op_X, 1);
    assign_char(nn_op_X, in_nn_f2_X, nR);
    cBYE(status);
  }
  else if ( ( in_nn_f1_X != NULL ) && ( in_nn_f2_X == NULL ) ) {
    status = open_temp_file(&nn_ofp, &nn_opfile);
    cBYE(status);
    fclose_if_non_null(nn_ofp);
    status = mk_file(nn_opfile, sizeof(char) * nR);
    cBYE(status);
    rs_mmap(nn_opfile, &nn_op_X, &n_nn_op_X, 1);
    assign_char(nn_op_X, in_nn_f1_X, nR);
    cBYE(status);
  }
  else {
    is_nn_field_needed = true;
    /* Need to and the 2 nn files */
    //--------------------------------------------
    // create an output file and mmap it 
    status = open_temp_file(&nn_ofp, &nn_opfile);
    cBYE(status);
    fclose_if_non_null(nn_ofp);
    n_nn_op_X = sizeof(char) * nR;
    status = mk_file(nn_opfile, n_nn_op_X);
    cBYE(status);
    status = rs_mmap(nn_opfile, &nn_op_X, &n_nn_op_X, 1);
    cBYE(status);
    //--------------------------------------------
  }
  //--------------------------------------------
  if ( ( nR <= MIN_ROWS_FOR_PARALLELISM ) || ( nT == 1 ) ) { 
    /* if one thread or if not enough data to justify threads */
    // fprintf(stderr, "nT = 1 or nR = [%lld] too small for parallelism \n", nR);
    nT = 1;
    void *vstat;
    vstat = f1f2opf3_core(0);
  }
  else {
    // Create threads
    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for ( int t = 0; t < nT; t++ ) { 
      // fprintf(stderr, "In main: creating thread %d\n", t);
      rc = pthread_create(&threads[t], NULL, f1f2opf3_core, (void *)t);
      if ( rc ) {
	fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
	go_BYE(-1);
      }
    }
    /* Free attribute and wait for the other threads */
    pthread_attr_destroy(&attr);
    for ( int t = 0; t < nT; t++ ) { 
      rc = pthread_join(threads[t], &thread_status);
      if ( rc ) { go_BYE(-1); }
    }
  }
  //--------------------------------------------
  *ptr_opfile = opfile;
  *ptr_nn_opfile = nn_opfile;
 BYE:
  rs_munmap(op_X, n_op_X);
  rs_munmap(nn_op_X, n_nn_op_X);
  fclose_if_non_null(ofp);
  fclose_if_non_null(nn_ofp);
  return(status);
}
