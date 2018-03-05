#include "sqliteInt.h"
#include "vdbeInt.h"

Op* do_hack(Mem *aMem, Op *aOp,
Op *pOp) {
	  Mem *pIn1 = &aMem[pOp->p1];
	  Mem *pIn3 = &aMem[pOp->p3];
	  assert( (pOp->p5 & SQLITE_AFF_MASK)>=SQLITE_AFF_NUMERIC );

	  if(pIn3->u.i <= pIn1->u.i){
		  pOp = &aOp[pOp->p2 - 1];
	  }
	  return pOp;
}
