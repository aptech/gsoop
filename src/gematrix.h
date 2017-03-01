#ifndef GEMATRIX_H
#define GEMATRIX_H

#include "gesymbol.h"
#include <stdio.h>

/**
 * GAUSS Matrix symbol type. Represents two dimensional array of double precision numbers. Matrices can
 * also be complex, storing both real and imaginary data.
 *
 */
class GAUSS_EXPORT GEMatrix : public GESymbol
{
public:
    GEMatrix();
    GEMatrix(double);
    GEMatrix(const vector<double> &data);
    GEMatrix(const vector<double> &data, int rows, int cols, bool complex = false);
    GEMatrix(const vector<double> &data, const vector<double> &imag_data, int rows, int cols);
    GEMatrix(const double *data, int rows, int cols, bool complex = false);
    GEMatrix(const double *data, const double *imag_data, int rows, int cols);

    bool setElement(double, bool imag = false);
    bool setElement(double, int row, int col, bool imag = false);

    double getElement(bool imag = false) const;
    double getElement(int row, int col, bool imag = false) const;

    vector<double> getData(bool imag = false) const;
    vector<double> getImagData() const;

    virtual void clear();
    virtual std::string toString() const;

//    using GESymbol::getRows;
//    using GESymbol::getCols;
//    using GESymbol::isComplex;

//    using GESymbol::setRows;
//    using GESymbol::setCols;
//    using GESymbol::setComplex;

private:
    GEMatrix(Matrix_t*);

    void Init(const vector<double> &data, int rows, int cols, bool complex = false);
    void Init(const vector<double> &real_data, const vector<double> &imag_data, int rows, int cols, bool complex = false);
    void Init(const double *real_data, const double *imag_data, int rows, int cols, bool complex = false); 

    double *data_;
    double *data_imag_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GEMATRIX_H
