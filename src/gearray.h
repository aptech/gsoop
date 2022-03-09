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
    GEArray(std::vector<int> orders, VECTOR_DATA(double) data, bool complex = false);
    GEArray(const int *orders, int orders_len, const double *data, int data_len, bool complex = false);

    GEMatrix* getPlane(std::vector<int> orders, bool imag = false) const;
    std::vector<double> getVector(std::vector<int> orders, bool imag = false) const;

    double getElement(std::vector<int> orders, bool imag = false) const;
    bool setElement(double, std::vector<int> orders, bool imag = false);

    std::vector<double> getData(bool imag = false) const;
    std::vector<double> getImagData() const;
    std::vector<int> getOrders() const;

    int getDimensions() const;
    virtual int size() const;

    virtual std::string toString() const;
    virtual void clear();

    Array_t* toInternal();

private:
    GEArray(Array_t*);
    bool Init(Array_t *);
    void Init(const int *orders, int orders_len, const double *data, int data_len, bool complex);
    size_t totalElements() const { return this->num_elements_ * (isComplex() ? 2: 1); }

    // Holds array data
    std::vector<double> data_;

    // Orders of array
    int dims_;
    size_t num_elements_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GEARRAY_H
