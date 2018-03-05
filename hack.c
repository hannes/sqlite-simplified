#include "sqliteInt.h"

#include "vdbeInt.h"

static void applyNumericAffinity(Mem *pRec, int bTryForInt){
  double rValue;
  i64 iValue;
  u8 enc = pRec->enc;
  assert( (pRec->flags & (MEM_Str|MEM_Int|MEM_Real))==MEM_Str );
  if( sqlite3AtoF(pRec->z, &rValue, pRec->n, enc)==0 ) return;
  if( 0==sqlite3Atoi64(pRec->z, &iValue, pRec->n, enc) ){
    pRec->u.i = iValue;
    pRec->flags |= MEM_Int;
  }else{
    pRec->u.r = rValue;
    pRec->flags |= MEM_Real;
    if( bTryForInt ) sqlite3VdbeIntegerAffinity(pRec);
  }
  /* TEXT->NUMERIC is many->one.  Hence, it is important to invalidate the
  ** string representation after computing a numeric equivalent, because the
  ** string representation might not be the canonical representation for the
  ** numeric value.  Ticket [343634942dd54ab57b7024] 2018-01-31. */
  pRec->flags &= ~MEM_Str;
}

Op* do_hack(Mem *aMem, Op *aOp,      /* Copy of p->aMem */
Op *pOp, u8 encoding) {
	int iCompare;
	int res, res2;      /* Result of the comparison of pIn1 against pIn3 */
	  char affinity;      /* Affinity to use for comparison */
	  u16 flags1;         /* Copy of initial value of pIn1->flags */
	  u16 flags3;         /* Copy of initial value of pIn3->flags */
	  Mem *pOut;
	  Mem *pIn1 = &aMem[pOp->p1];
	  Mem *pIn3 = &aMem[pOp->p3];
	  flags1 = pIn1->flags;
	  flags3 = pIn3->flags;
	  if( (flags1 | flags3)&MEM_Null ){
	    /* One or both operands are NULL */
	    if( pOp->p5 & SQLITE_NULLEQ ){
	      /* If SQLITE_NULLEQ is set (which will only happen if the operator is
	      ** OP_Eq or OP_Ne) then take the jump or not depending on whether
	      ** or not both operands are null.
	      */
	      assert( pOp->opcode==OP_Eq || pOp->opcode==OP_Ne );
	      assert( (flags1 & MEM_Cleared)==0 );
	      assert( (pOp->p5 & SQLITE_JUMPIFNULL)==0 );
	      if( (flags1&flags3&MEM_Null)!=0
	       && (flags3&MEM_Cleared)==0
	      ){
	        res = 0;  /* Operands are equal */
	      }else{
	        res = 1;  /* Operands are not equal */
	      }
	    }else{
	      /* SQLITE_NULLEQ is clear and at least one operand is NULL,
	      ** then the result is always NULL.
	      ** The jump is taken if the SQLITE_JUMPIFNULL bit is set.
	      */
	      if( pOp->p5 & SQLITE_STOREP2 ){
	        pOut = &aMem[pOp->p2];
	        iCompare = 1;    /* Operands are not equal */
	        MemSetTypeFlag(pOut, MEM_Null);
	      }else{
	        if( pOp->p5 & SQLITE_JUMPIFNULL ){
	          //goto jump_to_p2;
	        }
	      }
	      //break;
	    }
	  }else{
	    /* Neither operand is NULL.  Do a comparison. */
	    affinity = pOp->p5 & SQLITE_AFF_MASK;
	    if( affinity>=SQLITE_AFF_NUMERIC ){
	      if( (flags1 | flags3)&MEM_Str ){
	        if( (flags1 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){
	          applyNumericAffinity(pIn1,0);
	          testcase( flags3!=pIn3->flags ); /* Possible if pIn1==pIn3 */
	          flags3 = pIn3->flags;
	        }
	        if( (flags3 & (MEM_Int|MEM_Real|MEM_Str))==MEM_Str ){
	          applyNumericAffinity(pIn3,0);
	        }
	      }
	      /* Handle the common case of integer comparison here, as an
	      ** optimization, to avoid a call to sqlite3MemCompare() */
	      if( (pIn1->flags & pIn3->flags & MEM_Int)!=0 ){
	        if( pIn3->u.i > pIn1->u.i ){ res = +1; goto compare_op1; }
	        if( pIn3->u.i < pIn1->u.i ){ res = -1; goto compare_op1; }
	        res = 0;
	        goto compare_op1;
	      }
	    }else if( affinity==SQLITE_AFF_TEXT ){
	      if( (flags1 & MEM_Str)==0 && (flags1 & (MEM_Int|MEM_Real))!=0 ){
	        testcase( pIn1->flags & MEM_Int );
	        testcase( pIn1->flags & MEM_Real );
	        sqlite3VdbeMemStringify(pIn1, encoding, 1);
	        testcase( (flags1&MEM_Dyn) != (pIn1->flags&MEM_Dyn) );
	        flags1 = (pIn1->flags & ~MEM_TypeMask) | (flags1 & MEM_TypeMask);
	        assert( pIn1!=pIn3 );
	      }
	      if( (flags3 & MEM_Str)==0 && (flags3 & (MEM_Int|MEM_Real))!=0 ){
	        testcase( pIn3->flags & MEM_Int );
	        testcase( pIn3->flags & MEM_Real );
	        sqlite3VdbeMemStringify(pIn3, encoding, 1);
	        testcase( (flags3&MEM_Dyn) != (pIn3->flags&MEM_Dyn) );
	        flags3 = (pIn3->flags & ~MEM_TypeMask) | (flags3 & MEM_TypeMask);
	      }
	    }
	    assert( pOp->p4type==P4_COLLSEQ || pOp->p4.pColl==0 );
	    res = sqlite3MemCompare(pIn3, pIn1, pOp->p4.pColl);
	  }
	compare_op1:
	  /* At this point, res is negative, zero, or positive if reg[P1] is
	  ** less than, equal to, or greater than reg[P3], respectively.  Compute
	  ** the answer to this operator in res2, depending on what the comparison
	  ** operator actually is.  The next block of code depends on the fact
	  ** that the 6 comparison operators are consecutive integers in this
	  ** order:  NE, EQ, GT, LE, LT, GE */
	  assert( OP_Eq==OP_Ne+1 ); assert( OP_Gt==OP_Ne+2 ); assert( OP_Le==OP_Ne+3 );
	  assert( OP_Lt==OP_Ne+4 ); assert( OP_Ge==OP_Ne+5 );
	  if( res<0 ){                        /* ne, eq, gt, le, lt, ge */
	    static const unsigned char aLTb[] = { 1,  0,  0,  1,  1,  0 };
	    res2 = aLTb[pOp->opcode - OP_Ne];
	  }else if( res==0 ){
	    static const unsigned char aEQb[] = { 0,  1,  0,  1,  0,  1 };
	    res2 = aEQb[pOp->opcode - OP_Ne];
	  }else{
	    static const unsigned char aGTb[] = { 1,  0,  1,  0,  0,  1 };
	    res2 = aGTb[pOp->opcode - OP_Ne];
	  }

	  /* Undo any changes made by applyAffinity() to the input registers. */
	  assert( (pIn1->flags & MEM_Dyn) == (flags1 & MEM_Dyn) );
	  pIn1->flags = flags1;
	  assert( (pIn3->flags & MEM_Dyn) == (flags3 & MEM_Dyn) );
	  pIn3->flags = flags3;

	  if( pOp->p5 & SQLITE_STOREP2 ){
	    pOut = &aMem[pOp->p2];
	    iCompare = res;
	    if( (pOp->p5 & SQLITE_KEEPNULL)!=0 ){
	      /* The KEEPNULL flag prevents OP_Eq from overwriting a NULL with 1
	      ** and prevents OP_Ne from overwriting NULL with 0.  This flag
	      ** is only used in contexts where either:
	      **   (1) op==OP_Eq && (r[P2]==NULL || r[P2]==0)
	      **   (2) op==OP_Ne && (r[P2]==NULL || r[P2]==1)
	      ** Therefore it is not necessary to check the content of r[P2] for
	      ** NULL. */
	      assert( pOp->opcode==OP_Ne || pOp->opcode==OP_Eq );
	      assert( res2==0 || res2==1 );
	      testcase( res2==0 && pOp->opcode==OP_Eq );
	      testcase( res2==1 && pOp->opcode==OP_Eq );
	      testcase( res2==0 && pOp->opcode==OP_Ne );
	      testcase( res2==1 && pOp->opcode==OP_Ne );
	      //if( (pOp->opcode==OP_Eq)==res2 ) break;
	    }
	    MemSetTypeFlag(pOut, MEM_Int);
	    pOut->u.i = res2;
	  }else{
	    if( res2 ){
	    	  pOp = &aOp[pOp->p2 - 1];
	    }
	  }
	  return pOp;
}
