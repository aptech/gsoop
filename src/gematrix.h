#ifndef GEMATRIX_H
#define GEMATRIX_H

#include "gesymbol.h"
#include <stdio.h>
#include <memory>

/**
 * GAUSS Matrix symbol type. Represents two dimensional array of double precision numbers. Matrices can
 * also be complex, storing both real and imaginary data.
 *
 */
class GAUSS_EXPORT GEMatrix : public GESymbol
{
public:
    GEMatrix();
    virtual ~GEMatrix();
    GEMatrix(double);
    GEMatrix(VECTOR_DATA(double) data);
    GEMatrix(VECTOR_DATA(double) data, int rows, int cols, bool complex = false);
    GEMatrix(VECTOR_DATA(double) data, VECTOR_DATA(double) imag_data, int rows, int cols);
    GEMatrix(const double *data, int rows, int cols, bool complex = false);
    GEMatrix(const double *data, const double *imag_data, int rows, int cols);

    bool setElement(double value, bool imag = false);
    bool setElement(double value, int idx, bool imag = false);
    bool setElement(double value, int row, int col, bool imag = false);

    double getElement(bool imag = false) const;
    double getElement(int idx, bool imag = false) const;
    double getElement(int row, int col, bool imag = false) const;

    vector<double> getData(bool imag = false) const;
    vector<double> getImagData() const;

    virtual void clear();
    virtual std::string toString() const;

#ifdef SWIGPHP
    int position_;
#endif

private:
	GEMatrix(Matrix_t*);
    GEMatrix(GAUSS_MatrixInfo_t*);

    void Init(VECTOR_DATA(double) data, int rows, int cols, bool complex = false);
    void Init(VECTOR_DATA(double) real_data, VECTOR_DATA(double) imag_data, int rows, int cols, bool complex = false);
    void Init(const double *data, const double *imag_data, int rows, int cols, bool complex = false); 

	std::vector<double> data_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GEMATRIX_H
