#ifndef GESTRINGARRAY_H
#define GESTRINGARRAY_H

#include "gesymbol.h"
#include <stdio.h>
#include <vector>
#include <string>
using namespace std;

/**
 * GAUSS String Array Symbol type. Represents a standard string array. Data is stored
 * internally as a vector.
 *
 */
class GAUSS_EXPORT GEStringArray : public GESymbol
{
public:
    GEStringArray();
    GEStringArray(VECTOR_DATA(string) data);
    GEStringArray(VECTOR_DATA(string) data, int rows, int cols);
    void setData(VECTOR_DATA(string) data, int, int);
    bool setElement(const string &value, int index);
    bool setElement(const string &value, int row, int col);

    string getElement(int index) const;
    string getElement(int row, int col) const;
    vector<string> getData() const;

    virtual string toString() const;
    virtual int size() const { return data_.size(); }
	virtual void clear() { data_.clear(); data_.resize(1); setRows(1); setCols(1); }

    StringArray_t* toInternal();

#ifdef SWIGPHP
    int position_;
#endif

private:
    GEStringArray(StringArray_t*);
    bool fromStringArray(StringArray_t*);

    vector<string> data_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GESTRINGARRAY_H
