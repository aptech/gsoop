#ifndef GESYMBOL_H
#define GESYMBOL_H

#include <stdlib.h>
#include "gauss.h"
#include "gesymtype.h"
#include <string>
#include <iostream>
#include <vector>
#include <cstring>

#ifdef SWIGPHP
#define VECTOR_DATA_INIT(N,X,S) std::vector< X > *N = new std::vector< X >(S)
#define VECTOR_DATA(X) std::vector< X > *
#define VECTOR_VAR(X) X->
#define VECTOR_VAR_DELETE_CHECK(X) delete X
#else
#define VECTOR_DATA_INIT(N,X,S) std::vector< X > N(S)
#define VECTOR_DATA(X) const std::vector< X > &
#define VECTOR_VAR(X) X.
#define VECTOR_VAR_DELETE_CHECK(X)
#endif

class GAUSS_EXPORT doubleArray
{
public:
    doubleArray(int nelements) : rows_(nelements), cols_(1) { data_ = static_cast<double*>(GAUSS_Malloc(nelements * sizeof(double))); }
    doubleArray(double *data, int nelements) : data_(data), rows_(nelements), cols_(1) {}
    doubleArray(double *data, int rows, int cols) : data_(data), rows_(rows), cols_(cols) {}
    double getitem(int index) { return data_[index]; }
    void setitem(int index, double value) { data_[index] = value; }

    std::vector<double> getblock(int offset, int elements) { std::vector<double> ret(elements); memcpy(ret.data(), data_ + offset, elements * sizeof(double)); return ret; }
    std::vector<double> getrow(int row) { return getblock(row * cols_, cols_); }

    double* data() { return data_; }
    int rows() { return rows_; }
    int cols() { return cols_; }
    int size() { return rows_ * cols_; }

    void reset() { data_ = nullptr; rows_ = 0; cols_ = 0; }

#ifdef SWIGPHP
    int position_;
#endif

private:
    double *data_;
    int rows_;
    int cols_;
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

    virtual std::string toString() const { return std::string(); } /**< Returns a std::string representation of this object. */

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
