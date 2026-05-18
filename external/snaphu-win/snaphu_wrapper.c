/*************************************************************************

  snaphu Windows wrapper - provides a clean C API for calling SNAPHU
  from C++ code without file I/O.
  Single-tile mode only. Uses MCF/MST initialization only (no tree-solve).

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define main snaphu_main_disabled
#include "snaphu.c"
#include "snaphu_solver.c"
#include "snaphu_cost.c"
#include "snaphu_io.c"
#include "snaphu_util.c"
#include "snaphu_tile.c"
#include "snaphu_cs2.c"
#undef main


/* function: snaphu_unwrap()
 * -------------------------
 * Simplified wrapper: MCF or MST initialization + phase integration.
 * No tree-solve optimization loop (init result is used directly).
 *
 * Parameters:
 *   wrappedphase_in: input wrapped phase array (nrow x ncol), row-major
 *   mag_in:          input magnitude array (nrow x ncol), row-major, or NULL
 *   nrow:            number of rows
 *   ncol:            number of columns
 *   initmethod:      initialization method (1=MST, 2=MCF)
 *   costmode:        cost mode (1=TOPO, 2=DEFO, 3=SMOOTH)
 *   output:          output unwrapped phase array (nrow x ncol), row-major
 *
 * Returns 0 on success, non-zero on failure.
 */
int snaphu_unwrap(const float *wrappedphase_in,
                  const float *mag_in,
                  long nrow, long ncol,
                  int initmethod, int costmode,
                  float *output){

  paramT params[1];
  infileT infiles[1];
  outfileT outfiles[1];
  tileparamT tileparams[1];
  float **wrappedphase, **mag, **unwrappedphase;
  short **flows;
  void **costs;
  short **mstcosts;
  nodeT **nodes, ground[1];
  long row, col;

  if(wrappedphase_in == NULL || output == NULL || nrow < 2 || ncol < 2){
    return -1;
  }
  /* initialize structures */
  memset(infiles, 0, sizeof(infileT));
  memset(outfiles, 0, sizeof(outfileT));
  memset(tileparams, 0, sizeof(tileparamT));

  SetStreamPointers();
  SetDefaults(infiles, outfiles, params);

  params->initmethod = (signed char)initmethod;
  params->costmode = (signed char)costmode;
  params->ntilerow = 1;
  params->ntilecol = 1;
  infiles->infileformat = FLOAT_DATA;
  StrNCopy(infiles->infile, "memory", MAXSTRLEN);

  tileparams->firstrow = 0;
  tileparams->firstcol = 0;
  tileparams->nrow = nrow;
  tileparams->ncol = ncol;

  /* allocate and copy wrapped phase */
  wrappedphase = (float **)Get2DMem((int)nrow, (int)ncol,
                                     sizeof(float *), sizeof(float));
  if(wrappedphase == NULL) return -1;
  for(row = 0; row < nrow; row++)
    for(col = 0; col < ncol; col++)
      wrappedphase[row][col] = wrappedphase_in[row * ncol + col];

  /* allocate and copy magnitude */
  mag = (float **)Get2DMem((int)nrow, (int)ncol,
                            sizeof(float *), sizeof(float));
  if(mag == NULL){
    Free2DArray((void **)wrappedphase, (unsigned int)nrow);
    return -1;
  }
  if(mag_in != NULL){
    for(row = 0; row < nrow; row++)
      for(col = 0; col < ncol; col++)
        mag[row][col] = mag_in[row * ncol + col];
  }else{
    for(row = 0; row < nrow; row++)
      for(col = 0; col < ncol; col++)
        mag[row][col] = 1.0f;
  }

  /* build cost arrays (needed for init) */
  costs = NULL;
  mstcosts = NULL;
  BuildCostArrays(&costs, &mstcosts, mag, wrappedphase, NULL,
                  ncol, nrow, nrow, ncol, params, tileparams,
                  infiles, outfiles);

  SetGridNetworkFunctionPointers();

  /* initialize flows */
  /* Note: both MSTInitFlows and MCFInitFlows allocate their own flows
     array via flowsptr, so we don't pre-allocate. MSTInitFlows also
     frees mstcosts internally — we track this with a flag. */
  flows = NULL;
  nodes = NULL;
  int mstcosts_freed = 0;
  if(initmethod == MSTINIT){
    MSTInitFlows(wrappedphase, &flows, mstcosts, nrow, ncol,
                 &nodes, ground, params->initmaxflow);
    mstcosts_freed = 1;  /* MSTInitFlows frees mstcosts internally */
  }else{
    MCFInitFlows(wrappedphase, &flows, mstcosts, nrow, ncol,
                 params->cs2scalefactor);
    mstcosts_freed = 1;  /* SolveCS2 (called by MCFInitFlows) frees mstcosts */
  }

  /* integrate phase from flows */
  unwrappedphase = (float **)Get2DMem((int)nrow, (int)ncol,
                                       sizeof(float *), sizeof(float));
  if(unwrappedphase == NULL){
    if(!mstcosts_freed)
      Free2DArray((void **)mstcosts, 2 * (unsigned int)nrow - 1);
    Free2DArray((void **)costs, 2 * (unsigned int)nrow - 1);
    Free2DArray((void **)mag, (unsigned int)nrow);
    Free2DArray((void **)wrappedphase, (unsigned int)nrow);
    Free2DArray((void **)flows, 2 * (unsigned int)nrow - 1);
    return -1;
  }

  IntegratePhase(wrappedphase, unwrappedphase, flows, nrow, ncol);

  /* copy result to output (row-major) */
  for(row = 0; row < nrow; row++)
    for(col = 0; col < ncol; col++)
      output[row * ncol + col] = unwrappedphase[row][col];

  /* cleanup */
  Free2DArray((void **)unwrappedphase, (unsigned int)nrow);
  if(nodes != NULL){
    Free2DArray((void **)nodes, (unsigned int)(nrow - 1));
  }
  if(!mstcosts_freed)
    Free2DArray((void **)mstcosts, 2 * (unsigned int)nrow - 1);
  Free2DArray((void **)costs, 2 * (unsigned int)nrow - 1);
  Free2DArray((void **)mag, (unsigned int)nrow);
  Free2DArray((void **)wrappedphase, (unsigned int)nrow);
  Free2DArray((void **)flows, 2 * (unsigned int)nrow - 1);

  return 0;
}
