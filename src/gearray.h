#ifndef GEARRAY_H
#define GEARRAY_H

#include "gesymbol.h"

/**
 * GAUSS Array symbol type. This represents An N-dimensional array of double precision numbers.
 *
 */
class GAUSS_EXPORT GEArray : public GESymbol
{
public:
    GEArray();
    GEArray(const vector<int> &orders, const vector<double> &data, bool complex = false);
    GEArray(const int *orders, int orders_len, const double *data, int data_len, bool complex = false);

    GEMatrix* getPlane(const vector<int> &, bool imag = false) const;
    vector<double> getVector(const vector<int> &, bool imag = false) const;

    double getElement(const vector<int> &, bool imag = false) const;
    bool setElement(double, const vector<int> &, bool imag = false);

    vector<double> getData(bool imag = false) const;
    vector<double> getImagData() const;
    vector<int> getOrders() const;

    int getDimensions() const;
    virtual int size() const;

    virtual std::string toString() const;
    virtual void clear();

private:
    GEArray(Array_t*);
    bool Init(Array_t *);
    void Init(const int *orders, int orders_len, const double *data, int data_len, bool complex);
    int totalElements() const { return num_elements_ * (isComplex() ? 2: 1); }

    // Holds array data
    double *data_;

    // Orders of array
    int *orders_;
    int dims_;
    int num_elements_;

    friend class GAUSS;
};

#endif // GEARRAY_H
