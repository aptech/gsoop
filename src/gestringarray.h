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
class GEStringArray : public GESymbol
{
public:
    GEStringArray();
    GEStringArray(vector<string>);
    GEStringArray(vector<string>, int, int);

    string getElement(int, int);
    vector<string> getData();

    void setData(vector<string>, int, int);
    bool setElement(string, int, int);

    virtual string toString();
    virtual int size() { return data_.size(); }
    virtual void clear() { data_.clear(); setRows(1); setCols(1); }

private:
    GEStringArray(StringArray_t*);
    bool fromStringArray(StringArray_t*);

    vector<string> data_;

    friend class GAUSS;
};

#endif // GESTRINGARRAY_H
