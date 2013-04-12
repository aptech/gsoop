#ifndef GEARRAY_H
#define GEARRAY_H

#include "gesymbol.h"

class GEMatrix;

/**
 * GAUSS Array symbol type. This represents An N-dimensional array of double precision numbers.
 *
 */
class GEArray : public GESymbol
{
public:
    GEArray();
    GEArray(vector<int>, vector<double>, bool complex = false);

    GEMatrix* getPlane(vector<int>, bool imag = false);
    vector<double> getVector(vector<int>, bool imag = false);

    double getElement(vector<int>, bool imag = false);
    bool setElement(double, vector<int>, bool imag = false);

    vector<double> getData(bool imag = false);
    vector<double> getImagData();
    vector<int> getOrders();

    int getDimensions();
    virtual int size();

    virtual std::string toString();
    virtual void clear();

private:
    GEArray(Array_t*);
    bool Init(Array_t *);
    void Init(vector<int>, vector<double>, bool complex = false);
    int totalElements() { return num_elements_ * (isComplex() ? 2: 1); }

    // Holds array data
    double *data_;

    // Orders of array
    int *orders_;
    int dims_;
    int num_elements_;

    friend class GAUSS;
};

#endif // GEARRAY_H
