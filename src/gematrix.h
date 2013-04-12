#ifndef GEMATRIX_H
#define GEMATRIX_H

#include "gesymbol.h"
#include <stdio.h>

/**
 * GAUSS Matrix symbol type. Represents two dimensional array of double precision numbers. Matrices can
 * also be complex, storing both real and imaginary data.
 *
 */
class GEMatrix : public GESymbol
{
public:
    GEMatrix();
    GEMatrix(double);
    GEMatrix(vector<double>);
    GEMatrix(vector<double>, int rows, int cols, bool complex = false);
    GEMatrix(vector<double>, vector<double>, int rows, int cols);

    bool setElement(double, bool imag = false);
    bool setElement(double, int row, int col, bool imag = false);

    double getElement(bool imag = false);
    double getElement(int row, int col, bool imag = false);

    vector<double> getData(bool imag = false);
    vector<double> getImagData();

    virtual void clear();
    virtual std::string toString();

//    using GESymbol::getRows;
//    using GESymbol::getCols;
//    using GESymbol::isComplex;

//    using GESymbol::setRows;
//    using GESymbol::setCols;
//    using GESymbol::setComplex;

private:
    GEMatrix(Matrix_t*);

    void Init(vector<double> data, int rows, int cols, bool complex = false);
    void Init(vector<double> real_data, vector<double> imag_data, int rows, int cols, bool complex = false);

    double *data_;
    double *data_imag_;

    friend class GAUSS;
};

#endif // GEMATRIX_H
