#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <ctype.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(String) dgettext ("RxODE", String)
/* replace pkg as appropriate */
#else
#define _(String) (String)
#endif

#ifndef HAVE_STRCASECMP
#define HAVE_STRCASECMP 0
#endif

int casecmp(const char *s1, const char *s2) {
  register unsigned char u1, u2;
  for (;;) {
    u1 = (unsigned char) tolower(*s1++);
    u2 = (unsigned char) tolower(*s2++);
    if (u1 != u2) {
      return u1 - u2;
    }
    if (u1 == 0 || u2 == 0) {
      return 0;
    }
  }
}

static inline int isSingleInt(SEXP in, int defaultVal) {
  int type = TYPEOF(in);
  if (type == INTSXP && Rf_length(in) == 1) {
    if (!Rf_isMatrix(in)) return INTEGER(in)[0];
  } else if (type == REALSXP && Rf_length(in) == 1) {
    if (!Rf_isMatrix(in)) return (int)(REAL(in)[0]);
  }
  return defaultVal;
}

static inline int isSymNameMat(SEXP cur) {
  int type = TYPEOF(cur);
  if (type == INTSXP || type == REALSXP) {
    if (Rf_isMatrix(cur)){
      int nrows = Rf_nrows(cur);
      int ncols = Rf_ncols(cur);
      if (nrows == ncols) {
	SEXP dimn = Rf_getAttrib(cur, R_DimNamesSymbol);
	if (dimn != R_NilValue) {
	  return nrows;
	}
      }
    }
  }
  return 0;
}

int getCheckDim(SEXP lst, int i) {
  SEXP cur = VECTOR_ELT(lst, i);
  int type = TYPEOF(cur);
  int same=1;
  if (type == VECSXP) {
    if (Rf_length(cur) != 2){
      Rf_error(_("when repeating matrices you need to use 'list(mat, n)'"));
    }
    same = isSingleInt(VECTOR_ELT(cur, 1), NA_INTEGER);
    if (same == NA_INTEGER) {
      Rf_error(_("you can only repeat a matrix a single positive number of times"));
    }
    if (same <= 0) {
      Rf_error(_("you need to repeat a matrix a positive number of times"));
    }
    cur = VECTOR_ELT(cur, 0);
    type = TYPEOF(cur);
  }
  int ret = isSymNameMat(cur);
  if (ret){
    return ret*same;
  } else {
    Rf_error(_("list element %d is not a symmetric named matrix"), i+1);
  }
  return 0;
}

static inline int setStrElt(SEXP retN, SEXP colnames, int curBand, int j,
			    const char *fmt, int doFormat, int *cnt, int nsame) {
  if (doFormat && nsame > 1) {
    char out[100];
    int cx = snprintf( out, 100, fmt, cnt[0]++);
    SET_STRING_ELT(retN, curBand+j, Rf_mkChar(out));
    return cx;
  } else {
    SET_STRING_ELT(retN, curBand+j, STRING_ELT(colnames, j));
  }
  return 0;
}

SEXP getLotriProp(SEXP names, int i,
		  SEXP lotriProp,
		  SEXP lotriPropNames, const char *prop) {
  const char *what = CHAR(STRING_ELT(names, i));
  for (int j = Rf_length(lotriPropNames); j--;) {
    const char *cur = CHAR(STRING_ELT(lotriPropNames, j));
    if (!strcmp(what, cur)){
      SEXP lotriCur = VECTOR_ELT(lotriProp, j);
      SEXP lotriCurNames = Rf_getAttrib(lotriCur, R_NamesSymbol);
      for (int k = Rf_length(lotriCurNames); k--; ) {
	const char *cur2 = CHAR(STRING_ELT(lotriCurNames, k));
	if (!strcmp(cur2, prop)) {
	  return VECTOR_ELT(lotriCur, k);
	}
      }
    }
  }
  return R_NilValue;
}

int getSame(SEXP names, int i, SEXP lotriProp, SEXP lotriPropNames) {
  SEXP s = getLotriProp(names, i, lotriProp, lotriPropNames, "same");
  if (!Rf_isNull(s)) {
    return isSingleInt(s, 1);
  }
  return 1;
}

SEXP lotriToLstMat(SEXP lotri){
  SEXP lotriProp = Rf_getAttrib(lotri, Rf_install("lotri"));
  if (Rf_isNull(lotriProp)) {
    return lotri;
  }
  SEXP lotriNames = Rf_getAttrib(lotri, R_NamesSymbol);
  SEXP lotriPropNames = Rf_getAttrib(lotriProp, R_NamesSymbol);
  int pro=0;
  SEXP ret = PROTECT(Rf_allocVector(VECSXP, Rf_length(lotri))); pro++;
  int nsame;
  for (int i = Rf_length(ret); i--;) {
    nsame = getSame(lotriNames, i, lotriProp, lotriPropNames);
    if (nsame > 1){
      SEXP cur = PROTECT(Rf_allocVector(VECSXP, 2)); pro++;
      SET_VECTOR_ELT(cur, 0, VECTOR_ELT(lotri, i));
      SEXP ns = PROTECT(Rf_allocVector(INTSXP, 1)); pro++;
      INTEGER(ns)[0] = nsame;
      SET_VECTOR_ELT(cur, 1, ns);
      SET_VECTOR_ELT(ret, i, cur);
    } else {
      SET_VECTOR_ELT(ret, i, VECTOR_ELT(lotri, i));
    }
  }
  UNPROTECT(pro);
  return ret;
}

typedef struct lotriInfo {
  SEXP lst;
  int doFormat;
  const char *fmt;
  int counter;
  int pro;
} lotriInfo;

lotriInfo _lotriLstToMat0(SEXP lst_, SEXP format, SEXP startNum) {
  lotriInfo ret;
  ret.pro = 0;
  ret.lst = PROTECT(lotriToLstMat(lst_)); ret.pro++;
  int fmtType = TYPEOF(format);
  ret.doFormat = 0;
  if (fmtType == STRSXP && Rf_length(format) == 1) {
    ret.fmt = CHAR(STRING_ELT(format, 0));
    ret.doFormat=1;
  } else if (fmtType) {
    UNPROTECT(ret.pro);
    Rf_error(_("'format' must be a single length string or NULL"),
	     fmtType);
  } else {
    SEXP fmt2 = Rf_getAttrib(lst_, Rf_install("format"));
    if (TYPEOF(fmt2) == STRSXP && Rf_length(fmt2) == 1) {
      ret.fmt = CHAR(STRING_ELT(fmt2, 0));
      ret.doFormat=1;
    }
  }
  ret.counter = 0;
  if (ret.doFormat) {
    ret.counter = isSingleInt(startNum, NA_INTEGER);
    if (ret.counter == NA_INTEGER){
      SEXP startNum2 = Rf_getAttrib(lst_, Rf_install("start"));
      ret.counter = isSingleInt(startNum2, NA_INTEGER);
      if (ret.counter == NA_INTEGER) {
	UNPROTECT(ret.pro);
	Rf_error(_("when format is specified, 'startNum' must be a single integer"));
      }
    }
  }
  return ret;
}

SEXP _lotriLstToMat(SEXP lst_, SEXP format, SEXP startNum) {
  int type = TYPEOF(lst_), totN;
  if (type != VECSXP) {
    if (isSymNameMat(lst_)) {
      return lst_;
    }
    Rf_error(_("expects a list named symmetric matrices"));
  }
  lotriInfo li = _lotriLstToMat0(lst_, format, startNum);
  int len = Rf_length(li.lst);
  int totdim = 0;
  int i, j;
  if (len == 2) {
    int repN = isSingleInt(VECTOR_ELT(li.lst, 1), NA_INTEGER);
    if (repN == NA_INTEGER){
    } else if (repN > 0) {
      if (isSymNameMat(VECTOR_ELT(li.lst, 0))){
	SEXP new = PROTECT(Rf_allocVector(VECSXP, 1)); li.pro++;
	SET_VECTOR_ELT(new, 0, li.lst);
	SEXP ret = _lotriLstToMat(new, format, startNum);
	UNPROTECT(li.pro);
	return ret;
      }
    }
  }
  for (i = 0; i < len; ++i) {
    totdim += getCheckDim(li.lst, i);
  }
  SEXP ret = PROTECT(Rf_allocMatrix(REALSXP, totdim, totdim)); li.pro++;
  SEXP retN = PROTECT(Rf_allocVector(STRSXP, totdim)); li.pro++;
  double *retd = REAL(ret);
  // Initialize to zero
  memset(retd, 0, sizeof(double)*totdim*totdim);
  // Now use memcpy/ integer conversion to c
  SEXP cur;
  double *curd;
  int *curi;
  int curBand = 0;
  SEXP dimnames, colnames, sameS;
  int nsame;
  for (i = 0; i < len; ++i) {
    cur = VECTOR_ELT(li.lst, i);
    type = TYPEOF(cur);
    nsame = 1;
    if (type == VECSXP) {
      sameS = VECTOR_ELT(cur, 1);
      nsame = isSingleInt(sameS, 1);
      cur = VECTOR_ELT(cur, 0);
      type = TYPEOF(cur);
    }
    totN = Rf_ncols(cur);
    dimnames = Rf_getAttrib(cur, R_DimNamesSymbol);
    colnames = VECTOR_ELT(dimnames, 1);
    for (int cursame = nsame; cursame--;){
      if (type == REALSXP) {
	curd = REAL(cur);
	for (j = 0; j  < totN; ++j) {
	  memcpy(&retd[totdim*(curBand+j)+curBand],
		 &curd[totN*j], sizeof(double)*totN);
	  // Repeats dim names of repeated matrices
	  setStrElt(retN, colnames, curBand, j,
		    li.fmt, li.doFormat, &li.counter, nsame);
	}
      } else {
	curi = INTEGER(cur);
	for (j = 0; j < totN; ++j) {
	  double *to = &retd[totdim*(curBand+j)+curBand];
	  double *last = to + totN; // N - count
	  int *from = &curi[totN*j];
	  while (to != last) {
	    *(to++) = (double)(*(from++));
	  }
	  setStrElt(retN, colnames, curBand, j,
		    li.fmt, li.doFormat, &li.counter, nsame);
	}
      }
      curBand += totN;
    }
  }
  dimnames = PROTECT(Rf_allocVector(VECSXP, 2)); li.pro++;
  SET_VECTOR_ELT(dimnames, 0, retN);
  SET_VECTOR_ELT(dimnames, 1, retN);
  Rf_setAttrib(ret, R_DimNamesSymbol, dimnames);
  UNPROTECT(li.pro);
  return ret;
}

double getDouble(SEXP colnames, int i, SEXP inUpperLower, SEXP upperLowerNames,
		 double defaultValue, int type) {
  const char *lookup = CHAR(STRING_ELT(colnames, i));
  const char *current;
  for (int j = Rf_length(upperLowerNames); j--;) {
    current = CHAR(STRING_ELT(upperLowerNames, j));
    if (!strcmp(current, lookup)){
      return REAL(inUpperLower)[j];
    }
  }
  return defaultValue;
}

void setUpperLower(SEXP inUpperLower, SEXP colnames,
		   double *outUpperLower, int i0, double defaultValue,
		   const char *what, int pro0, int nsame) {
  SEXP upperLowerNames = Rf_getAttrib(inUpperLower, R_NamesSymbol);
  double value = defaultValue;
  int ncol = Rf_length(colnames);
  if (Rf_isNull(upperLowerNames)){
    if (Rf_length(inUpperLower) == 1) {
      int typ = TYPEOF(inUpperLower);
      if (typ == REALSXP) {
	value = REAL(inUpperLower)[0];
      } else if (typ == INTSXP) {
	value = (double)(INTEGER(inUpperLower)[0]);
      }
    } else if (Rf_length(inUpperLower) != 0) {
      UNPROTECT(pro0);
      Rf_error(_("cannot figure out valid '%s' properties"), what);
    }
    for (int i = ncol*nsame; i--;) {
      outUpperLower[i0+i] = value;
    }      
  } else {
    int typ = TYPEOF(inUpperLower);
    for (int i = ncol; i--;) {
      outUpperLower[i0+i] = getDouble(colnames, i, inUpperLower,
				      upperLowerNames, defaultValue, typ);
    }
    for (int i = 1; i < nsame; ++i) {
      memcpy(&outUpperLower[i0+i*ncol], &outUpperLower[i0], ncol*sizeof(double));
    }
  }
}
SEXP _lotriAllNames(SEXP lotri);

SEXP _lotriGetBounds(SEXP lst_, SEXP format, SEXP startNum) {
  int type = TYPEOF(lst_), totN;
  if (type != VECSXP) {
    Rf_error(_("expects lotri matrix"));
  }
  SEXP lotriProp = Rf_getAttrib(lst_, Rf_install("lotri"));
  SEXP lotriPropNames = Rf_getAttrib(lotriProp, R_NamesSymbol);
  SEXP names = Rf_getAttrib(lst_, R_NamesSymbol);
  if (Rf_isNull(lotriProp)) {
    SEXP names = _lotriAllNames(lst_);
    int len = Rf_length(names);
    int pro = 0;
    SEXP boundLower = PROTECT(Rf_allocVector(REALSXP, len)); pro++;
    SEXP boundUpper = PROTECT(Rf_allocVector(REALSXP, len)); pro++;
    Rf_setAttrib(boundLower, R_NamesSymbol, names);
    Rf_setAttrib(boundUpper, R_NamesSymbol, names);
    double *bL = REAL(boundLower);
    double *bU = REAL(boundUpper);
    for (int j = len; j--; ){
      bL[j] = R_NegInf;
      bU[j] = R_PosInf;
    }
    SEXP ret = PROTECT(Rf_allocVector(VECSXP, 2)); pro++;
    SET_VECTOR_ELT(ret, 0, boundLower);
    SET_VECTOR_ELT(ret, 1, boundUpper);
    SEXP retFN = PROTECT(Rf_allocVector(STRSXP, 2)); pro++;
    SET_STRING_ELT(retFN, 0, Rf_mkChar("lower"));
    SET_STRING_ELT(retFN, 1, Rf_mkChar("upper"));
    Rf_setAttrib(ret, R_NamesSymbol, retFN);
    UNPROTECT(pro);
    return ret;
    Rf_error(_("only works with lotri matrices"));
  }
  
  lotriInfo li = _lotriLstToMat0(lst_, format, startNum);
  int len = Rf_length(li.lst);
  int totdim=0;
  for (int i = 0; i < len; ++i) {
    totdim += getCheckDim(li.lst, i);
  }
  SEXP retN = PROTECT(Rf_allocVector(STRSXP, totdim)); li.pro++;
  SEXP boundLower = PROTECT(Rf_allocVector(REALSXP, totdim)); li.pro++;
  double *boundLowerD = REAL(boundLower);
  SEXP boundUpper = PROTECT(Rf_allocVector(REALSXP, totdim)); li.pro++;
  double *boundUpperD = REAL(boundUpper);
  int nsame, j;
  SEXP cur, sameS, dimnames, colnames;
  int curBand = 0;
  for (int i = 0; i < len; ++i) {
    cur = VECTOR_ELT(li.lst, i);
    type = TYPEOF(cur);
    nsame = 1;
    if (type == VECSXP) {
      sameS = VECTOR_ELT(cur, 1);
      nsame = isSingleInt(sameS, 1);
      cur = VECTOR_ELT(cur, 0);
      type = TYPEOF(cur);
    }
    totN = Rf_ncols(cur);
    dimnames = Rf_getAttrib(cur, R_DimNamesSymbol);
    colnames = VECTOR_ELT(dimnames, 1);
    SEXP upper = getLotriProp(names, i, lotriProp, lotriPropNames, "upper");
    setUpperLower(upper, colnames, boundUpperD, curBand, R_PosInf,
		  "upper", li.pro, nsame);
    SEXP lower = getLotriProp(names, i, lotriProp, lotriPropNames, "lower");
    setUpperLower(lower, colnames, boundLowerD, curBand, R_NegInf,
		  "lower", li.pro, nsame);
    for (int cursame = nsame; cursame--;){
      // Repeats dim names of repeated matrices
      for (j = 0; j  < totN; ++j) {
	setStrElt(retN, colnames, curBand, j,
		  li.fmt, li.doFormat, &li.counter, nsame);
      }
      curBand += totN;
    }
  }
  Rf_setAttrib(boundLower, R_NamesSymbol, retN);
  Rf_setAttrib(boundUpper, R_NamesSymbol, retN);
  SEXP ret = PROTECT(Rf_allocVector(VECSXP, 2)); li.pro++;
  SET_VECTOR_ELT(ret, 0, boundLower);
  SET_VECTOR_ELT(ret, 1, boundUpper);
  SEXP retFN = PROTECT(Rf_allocVector(STRSXP, 2)); li.pro++;
  SET_STRING_ELT(retFN, 0, Rf_mkChar("lower"));
  SET_STRING_ELT(retFN, 1, Rf_mkChar("upper"));
  Rf_setAttrib(ret, R_NamesSymbol, retFN);
  UNPROTECT(li.pro);
  return ret;
}

SEXP ampDefault(SEXP cur, SEXP dimn, double val, int pro0, const char * what) {
  if (TYPEOF(cur) != REALSXP) {
    UNPROTECT(pro0);
    Rf_error("'%s' needs to be a double", what);
  }
  int pro = 0;
  SEXP names = Rf_getAttrib(cur, R_NamesSymbol);
  int nDim = Rf_xlength(dimn);
  if (Rf_isNull(names)) {
    if (Rf_xlength(cur) == 1){
      SEXP ret = PROTECT(Rf_allocVector(REALSXP, nDim)); pro++;
      double *retD = REAL(ret);
      Rf_setAttrib(ret, R_NamesSymbol, dimn);
      double val = REAL(cur)[0];
      for (int i = nDim;i--;){
	retD[i] = val;
      }
      UNPROTECT(pro);
      return ret;
    } else {
      UNPROTECT(pro0);
      Rf_error("'%s' needs to be named", what);
    }
  } else {
    int nnames = Rf_xlength(names);
    SEXP ret = PROTECT(Rf_allocVector(REALSXP, nDim)); pro++;
    double *retD = REAL(ret);
    double *in = REAL(cur);
    for (int i=0; i < nDim; ++i) {
      int found = 0;
      for (int j = 0; j < nnames; ++j) {
	if (!strcmp(CHAR(STRING_ELT(dimn, i)),
		    CHAR(STRING_ELT(names, j)))) {
	  retD[i] = in[j];
	  found = 1;
	  break;
	}
      }
      if (found == 0) {
	retD[i] = val;
      }
    }
    Rf_setAttrib(ret, R_NamesSymbol, dimn);
    UNPROTECT(pro);
    return ret;
  }
  return R_NilValue;
}

// put into C to allow calling from RxODE from C.
SEXP _asLotriMat(SEXP x, SEXP extra, SEXP def){
  if (TYPEOF(def) != STRSXP || Rf_length(def) != 1) {
    Rf_error(_("'default' must be a 'string' of length 1"));
  }  
  if (!Rf_isMatrix(x)) {
    Rf_error(_("'x' needs to be a completely named matrix"));
  }
  SEXP dims = Rf_getAttrib(x, R_DimNamesSymbol);
  if (Rf_isNull(dims)){
    Rf_error(_("'x' needs to be a completely named matrix"));
  }
  SEXP dimn = VECTOR_ELT(dims, 0);
  if (Rf_isNull(dimn)) {
    Rf_error(_("'x' needs to be a completely named matrix"));
  }
  if (Rf_isNull(VECTOR_ELT(dims, 1))) {
    Rf_error(_("'x' needs to be a completely named matrix"));
  }
  const char *defVal = CHAR(STRING_ELT(def, 0));
  if (TYPEOF(extra) != VECSXP) {
    Rf_error(_("'extra' must be a list"));
  }
  int pro = 0;
  SEXP ret = PROTECT(Rf_allocVector(VECSXP, 1)); pro++;
  SET_VECTOR_ELT(ret, 0, x);
  Rf_setAttrib(ret, R_NamesSymbol, def);
  SEXP lotriClass = PROTECT(Rf_allocVector(STRSXP, 1)); pro++;
  SET_STRING_ELT(lotriClass, 0, Rf_mkChar("lotri"));
  
  // Now setup extra
  int nNull = 0;
  int lExtra = Rf_length(extra);
  if (lExtra == 0) {
    // Here extra isn't added.
    Rf_setAttrib(ret, R_ClassSymbol, lotriClass);
    UNPROTECT(pro);
    return ret;
  } else if (!strcmp(defVal, "")) {
    UNPROTECT(pro);
    Rf_error("extra properties need default try 'lotri(matrix,x=3,default=\"id\")'");
  }
  SEXP extraNames = Rf_getAttrib(extra, R_NamesSymbol);
  for (int i = lExtra; i--;) {
    if (Rf_isNull(VECTOR_ELT(extra, i))) {
      nNull++;
    }
  }
  SEXP lotri = PROTECT(Rf_allocVector(VECSXP, 1)); pro++;
  SEXP fextra = PROTECT(Rf_allocVector(VECSXP, lExtra-nNull)); pro++;
  Rf_setAttrib(lotri, R_NamesSymbol, def);
  SEXP fextraN = PROTECT(Rf_allocVector(STRSXP, lExtra-nNull)); pro++;
  int j = 0;
  for (int i = lExtra; i--;) {
    if (!Rf_isNull(VECTOR_ELT(extra, i))){
      SEXP curNameS = STRING_ELT(extraNames, i);
      const char *curName = CHAR(curNameS);
      if (!strcmp("lower", curName)) {
	SET_VECTOR_ELT(fextra, j, ampDefault(VECTOR_ELT(extra, i), dimn, R_NegInf, pro, "lower"));
      } else if (!strcmp("upper", curName)) {
	SET_VECTOR_ELT(fextra, j, ampDefault(VECTOR_ELT(extra, i), dimn, R_PosInf, pro, "upper"));
      } else {
	SET_VECTOR_ELT(fextra, j, VECTOR_ELT(extra, i));
      }
      SET_STRING_ELT(fextraN, j, curNameS);
      j++;
    }
  }
  SET_VECTOR_ELT(lotri, 0, fextra);
  Rf_setAttrib(fextra, R_NamesSymbol, fextraN);
  Rf_setAttrib(ret, Rf_install("lotri"), lotri);
  Rf_setAttrib(ret, R_ClassSymbol, lotriClass);
  UNPROTECT(pro);
  return ret;
}

SEXP getNestLotri(int lenNest, int extra, int pro0, int lotriLen,
		   SEXP nestN, SEXP lotri, SEXP names, SEXP lotri0, SEXP lotri0names,
		   SEXP sameC, const char *what, int *nestI, SEXP nestStart){
  if (TYPEOF(nestStart) != INTSXP || Rf_length(nestStart) != 1) {
    UNPROTECT(pro0);
    Rf_error("'%sStart' needs to be an 'integer' of length 1", what);
  }
  int pro = 0;
  SEXP nestLotri = PROTECT(Rf_allocVector(VECSXP, lenNest+extra)); pro++;
  SEXP nestLotriProp = PROTECT(Rf_allocVector(VECSXP, lenNest +extra)); pro++;
  SEXP lotriClass = PROTECT(Rf_allocVector(STRSXP, 1)); pro++;
  SET_STRING_ELT(lotriClass, 0, Rf_mkChar("lotri"));
  Rf_setAttrib(nestLotri, Rf_install("lotri"), nestLotriProp);
  SEXP nestN2;
  if (extra) {
    // Look for ID
    int found1=0, found2=0;
    nestN2 = PROTECT(Rf_allocVector(STRSXP, lenNest+extra)); pro++;
    for (int j = 0; j < lotriLen; ++j) {
      if (found1 == 0 &&
	  !casecmp("id", CHAR(STRING_ELT(names, j)))) {
	found1 = 1;
	SET_STRING_ELT(nestN2, 0, STRING_ELT(names, j));
	SET_VECTOR_ELT(nestLotri, 0, VECTOR_ELT(lotri, j));
      }
      if (found2 == 0 &&
	  !casecmp("id", CHAR(STRING_ELT(lotri0names, j)))) {
	SET_VECTOR_ELT(nestLotriProp, 0, VECTOR_ELT(lotri0, j));
	found2 = 1;
      }
      if (found1 == 1 && found2 == 1) {
	break;
      }
    }
    if (found1 == 0 || found2 == 0) {
      UNPROTECT(pro+pro0);
      Rf_error("'id' not found in 'lotri' matrix");
    }
    for (int i = 0; i  < lenNest; ++i) {
      SET_STRING_ELT(nestN2, i+1, STRING_ELT(nestN, i));
    }
  } else {
    nestN2 = nestN;
  }
  Rf_setAttrib(nestLotri, R_NamesSymbol, nestN2);
  Rf_setAttrib(nestLotriProp, R_NamesSymbol, nestN2);
  for (int i = extra; i < lenNest+extra; ++i) {
    int found1 = 0, found2 = 0;
    for (int j = 0; j < lotriLen; ++j) {
      const char *curNest = CHAR(STRING_ELT(nestN2, i));
      if (found1 == 0 &&
	  !strcmp(curNest, CHAR(STRING_ELT(names, j)))) {
	found1 = 1;
	SET_VECTOR_ELT(nestLotri, i, VECTOR_ELT(lotri, j));
      }
      if (found2 == 0 &&
	  !strcmp(curNest, CHAR(STRING_ELT(lotri0names, j)))) {
	// Here we found the lotri property,
	// Create a new list with "same" at the end
	SEXP curProp  = VECTOR_ELT(lotri0, i);
	int curPropN = Rf_length(curProp);
	SEXP curPropS = Rf_getAttrib(curProp, R_NamesSymbol);
	SEXP newProp  = PROTECT(Rf_allocVector(VECSXP, Rf_length(curProp)+1)); pro++;
	SEXP newPropS = PROTECT(Rf_allocVector(STRSXP, Rf_length(curProp)+1)); pro++;
	for (int k = 0; k < curPropN; ++k) {
	  SET_VECTOR_ELT(newProp, k, VECTOR_ELT(curProp, k));
	  SET_STRING_ELT(newPropS, k, STRING_ELT(curPropS, k));
	}
	SET_STRING_ELT(newPropS, curPropN, sameC);
	SEXP nestVal = PROTECT(Rf_allocVector(INTSXP, 1)); pro++;
	INTEGER(nestVal)[0] = nestI[i-extra];
	SET_VECTOR_ELT(newProp, curPropN, nestVal);
	Rf_setAttrib(newProp, R_NamesSymbol, newPropS);
	// Now assign it
	SET_VECTOR_ELT(nestLotriProp, i, newProp);
	found2 = 1;
      }
      if (found1 == 1 && found2 == 1) {
	break;
      }
    }
    if (found1 == 0 || found2 == 0) {
      UNPROTECT(pro+pro0);
      Rf_error("'%s' names do not match 'lotri' matrix", what);
    }
  }
  SEXP format = PROTECT(Rf_allocVector(STRSXP, 1)); pro++;
  if (extra) {
    SET_STRING_ELT(format, 0, Rf_mkChar("ETA[%d]"));
  } else {
    SET_STRING_ELT(format, 0, Rf_mkChar("THETA[%d]"));
  }
  Rf_setAttrib(nestLotri, R_ClassSymbol, lotriClass);
  Rf_setAttrib(nestLotri, Rf_install("format"), format);
  Rf_setAttrib(nestLotri, Rf_install("start"), nestStart);
  UNPROTECT(pro);
  return nestLotri;
}

SEXP blankProp(SEXP names){
  int pro = 0;
  SEXP lotriProp = PROTECT(Rf_allocVector(VECSXP, Rf_length(names)));pro++;
  for (int j = Rf_length(names); j--;) {
    SET_VECTOR_ELT(lotriProp, j, PROTECT(Rf_allocVector(VECSXP, 0))); pro++;
  }
  Rf_setAttrib(lotriProp, R_NamesSymbol, names);
  UNPROTECT(pro);
  return lotriProp;
}


SEXP _lotriSep(SEXP lotri, SEXP above, SEXP below,
	       SEXP aboveStart, SEXP belowStart) {
  int pro    = 0;
  SEXP names = Rf_getAttrib(lotri, R_NamesSymbol);
  SEXP lotri0 = Rf_getAttrib(lotri, Rf_install("lotri"));
  if (Rf_isNull(lotri0)) {
    lotri0 = blankProp(names);
  }
  SEXP lotri0names = Rf_getAttrib(lotri0, R_NamesSymbol);
  int lotriLen = Rf_length(names);
  if (lotriLen != Rf_length(lotri0)) {
    Rf_error("'lotri' malformed");
  }
  SEXP aboveN;
  SEXP belowN = Rf_getAttrib(below, R_NamesSymbol);
  if (Rf_isNull(belowN)){
    Rf_error("'below' needs to be named");
  }
  if (TYPEOF(below) != INTSXP) {
    Rf_error("'below' needs to be an integer");
  }
  int *belowI = INTEGER(below);
  int *aboveI;
  int lenAbove = Rf_length(above);
  int lenBelow = Rf_length(below);
  SEXP ret = PROTECT(Rf_allocVector(VECSXP, 2)); pro++;
  SEXP retN = PROTECT(Rf_allocVector(STRSXP, 2)); pro++;
  SET_STRING_ELT(retN, 0, Rf_mkChar("above"));
  SET_STRING_ELT(retN, 1, Rf_mkChar("below"));
  SEXP sameC = PROTECT(Rf_mkChar("same")); pro++;
  Rf_setAttrib(ret, R_NamesSymbol, retN);
  if (lenAbove == 0) {
    SET_VECTOR_ELT(ret, 0, R_NilValue);
  } else {
    aboveN = Rf_getAttrib(above, R_NamesSymbol);
    if (Rf_isNull(aboveN)){
      Rf_error("'above' needs to be named");
    }  
    if (TYPEOF(above) != INTSXP) {
      Rf_error("'above' needs to be an integer");
    }
    aboveI = INTEGER(above);    

    SET_VECTOR_ELT(ret, 0,
		   getNestLotri(lenAbove, 0, pro, lotriLen,
				 aboveN, lotri, names, lotri0, lotri0names,
				 sameC, "above", aboveI, aboveStart));
  }

  SET_VECTOR_ELT(ret, 1,
		   getNestLotri(lenBelow, 1, pro, lotriLen,
				 belowN, lotri, names, lotri0, lotri0names,
				 sameC, "below", belowI, belowStart));
  
  UNPROTECT(pro);
  return ret;
}

SEXP _lotriAllNames(SEXP lotri) {
  int pro = 0;
  if (Rf_isMatrix(lotri)){
    SEXP dimn = Rf_getAttrib(lotri, R_DimNamesSymbol);
    if (dimn == R_NilValue) {
      SEXP retN = PROTECT(Rf_allocVector(STRSXP, 0)); pro++;
      UNPROTECT(pro);
      return retN;
    }
    SEXP colnames = VECTOR_ELT(dimn, 1);
    if (Rf_isNull(colnames)){
      colnames = VECTOR_ELT(dimn, 0);
      if (Rf_isNull(colnames)) {
	SEXP retN = PROTECT(Rf_allocVector(STRSXP, 0)); pro++;
	UNPROTECT(pro);
	return retN;
      }
    }
    return colnames;
  } else {
    int type = TYPEOF(lotri);
    if (type == VECSXP) {
      int intN=0;
      for (int i = Rf_length(lotri); i--;){
	intN +=
	  Rf_length(VECTOR_ELT(Rf_getAttrib(VECTOR_ELT(lotri, i),
					    R_DimNamesSymbol), 1));
      }
      int j = 0;
      SEXP ret = PROTECT(Rf_allocVector(STRSXP, intN)); pro++;
      for (int i = Rf_length(lotri); i--;){
	SEXP cur = VECTOR_ELT(Rf_getAttrib(VECTOR_ELT(lotri, i),
					   R_DimNamesSymbol), 1);
	for (int k = 0; k < Rf_length(cur); ++k) {
	  /* SET_STRING_ELT(retN, curBand+j, Rf_mkChar(out)); */
	  SET_STRING_ELT(ret, j++, STRING_ELT(cur, k));
	}
      }
      UNPROTECT(pro);
      return ret;
    } else {
      Rf_error(_("not a matrix or lotri matrix"));
    }
  }
}

SEXP _lotriMaxNu(SEXP lotri) {
  SEXP lotriProp = Rf_getAttrib(lotri, Rf_install("lotri"));
  SEXP ret = PROTECT(Rf_allocVector(REALSXP, 1));
  REAL(ret)[0] = 0.0;
  if (Rf_isNull(lotriProp)){
    UNPROTECT(1);
    return ret;
  }
  SEXP lotriPropNames = Rf_getAttrib(lotriProp, R_NamesSymbol);
  SEXP names = Rf_getAttrib(lotri, R_NamesSymbol);
  double maxNu = 0.0;
  for (int i = Rf_length(lotri); i--;) {
    SEXP nu = getLotriProp(names, i, lotriProp, lotriPropNames, "nu");
    if (!Rf_isNull(nu) && Rf_length(nu) == 1){
      double tmp=0;
      if (TYPEOF(nu) == REALSXP && maxNu < (tmp = REAL(nu)[0])) {
	maxNu = tmp;
      }
    }
  }
  REAL(ret)[0] = maxNu;
  UNPROTECT(1);
  return ret;
}

SEXP _isLotri(SEXP lotri) {
  SEXP lotriProp = Rf_getAttrib(lotri, Rf_install("lotri"));
  SEXP ret = PROTECT(Rf_allocVector(LGLSXP, 1)); 
  if (Rf_isNull(lotriProp)) {
    if (TYPEOF(lotri) == VECSXP){
      int isL = 1;
      for (int i = Rf_length(lotri); i--;){
	SEXP cur = VECTOR_ELT(lotri, i);
	if (Rf_isMatrix(cur)) {
	  SEXP dimnames = Rf_getAttrib(cur, R_DimNamesSymbol);
	  if (Rf_isNull(dimnames)) {
	    isL = 0;
	    break;
	  }
	} else {
	  isL = 0;
	  break;
	}
      }
      INTEGER(ret)[0] = isL;
    } else {
      INTEGER(ret)[0] = 0;
    }
  } else {
    INTEGER(ret)[0] = 1;
  }
  UNPROTECT(1);
  return ret;
}

void R_init_lotri(DllInfo *info){
  R_CallMethodDef callMethods[]  = {
    {"_lotriLstToMat", (DL_FUNC) &_lotriLstToMat, 3},
    {"_asLotriMat", (DL_FUNC) &_asLotriMat, 3},
    {"_lotriSep", (DL_FUNC) &_lotriSep, 5},
    {"_lotriAllNames", (DL_FUNC) &_lotriAllNames, 1},
    {"_lotriGetBounds", (DL_FUNC) &_lotriGetBounds, 3},
    {"_lotriMaxNu", (DL_FUNC) &_lotriMaxNu, 1},
    {"_isLotri", (DL_FUNC) &_isLotri, 1},
    {NULL, NULL, 0}
  };
  R_RegisterCCallable("lotri", "_lotriLstToMat", (DL_FUNC) _lotriLstToMat);
  R_RegisterCCallable("lotri", "_asLotriMat", (DL_FUNC) _asLotriMat);
  R_RegisterCCallable("lotri", "_lotriSep", (DL_FUNC) _lotriSep);
  R_RegisterCCallable("lotri", "_lotriAllNames", (DL_FUNC) _lotriAllNames);
  R_RegisterCCallable("lotri", "_lotriGetBounds", (DL_FUNC) _lotriGetBounds);
  R_RegisterCCallable("lotri", "_lotriMaxNu", (DL_FUNC) _lotriMaxNu);
  R_RegisterCCallable("lotri", "_isLotri", (DL_FUNC) _isLotri);
  R_registerRoutines(info, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(info, FALSE);
}

