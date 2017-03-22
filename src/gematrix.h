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
    GEMatrix(const std::vector<double> &data);
    GEMatrix(const std::vector<double> &data, int rows, int cols, bool complex = false);
    GEMatrix(const std::vector<double> &data, const std::vector<double> &imag_data, int rows, int cols);
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

private:
	GEMatrix(Matrix_t*);
    GEMatrix(GAUSS_MatrixInfo_t*);

    void Init(const std::vector<double> &data, int rows, int cols, bool complex = false);
    void Init(const std::vector<double> &real_data, const std::vector<double> &imag_data, int rows, int cols, bool complex = false);
    void Init(const double *data, const double *imag_data, int rows, int cols, bool complex = false); 

	std::vector<double> data_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GEMATRIX_H
