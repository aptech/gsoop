#ifndef GESYMBOL_H
#define GESYMBOL_H

#include <stdlib.h>
#include "gauss.h"
#include "gesymtype.h"
#include <string>
#include <iostream>
#include <vector>
using namespace std;

#ifdef SWIGPHP
#define VECTOR_DATA_INIT(N,X,S) vector< X > *N = new vector< X >(S)
#define VECTOR_DATA(X) vector< X > *
#define VECTOR_VAR(X) X->
#define VECTOR_VAR_DELETE_CHECK(X) delete X
#else
#define VECTOR_DATA_INIT(N,X,S) vector< X > N(S)
#define VECTOR_DATA(X) const vector< X > &
#define VECTOR_VAR(X) X.
#define VECTOR_VAR_DELETE_CHECK(X)
#endif

class GAUSS_EXPORT doubleArray
{
public:
    doubleArray(int nelements) : elements_(nelements) { data_ = static_cast<double*>(GAUSS_Malloc(nelements * sizeof(double))); }
    doubleArray(double *data, int nelements) : data_(data), elements_(nelements) {}
    double getitem(int index) { return data_[index]; }
    void setitem(int index, double value) { data_[index] = value; }

    double* data() { return data_; }
    int size() { return elements_; }

    void reset() { data_ = nullptr; elements_ = 0; }

#ifdef SWIGPHP
    int position_;
#endif

private:
    double *data_;
    int elements_;
};

/**
  * Abstract parent class for all symbol types.
  */
class GAUSS_EXPORT GESymbol
{
public:
    virtual int getRows() const;      /**< Return row count. */
    virtual int getCols() const;      /**< Return column count. */
    virtual bool isComplex() const;   /**< Return if data is complex. Applies to GEArray and GEMatrix only. */

    virtual int size() const;         /**< Return element count. */
	virtual void clear() { rows_ = 1; cols_ = 1; complex_ = false; }     /**< Clear all corresponding symbol data. Does not clear from workspace. */

    virtual string toString() const { return string(); } /**< Returns a string representation of this object. */

    int type() const { return type_; }

protected:
    GESymbol(int type);
    virtual ~GESymbol();

    virtual void setRows(int);
    virtual void setCols(int);
    virtual void setComplex(bool);

    int rows_;
    int cols_;
    bool complex_;

    int type_;
};

#endif // GESYMBOL_H
