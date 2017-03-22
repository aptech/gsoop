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
    GEStringArray(const vector<string> &);
    GEStringArray(const vector<string> &, int, int);

    string getElement(int, int) const;
    vector<string> getData() const;

    void setData(const vector<string> &, int, int);
    bool setElement(const string &, int, int);

    virtual string toString() const;
    virtual int size() const { return data_.size(); }
	virtual void clear() { data_.clear(); data_.resize(1); setRows(1); setCols(1); }

private:
    GEStringArray(StringArray_t*);
    bool fromStringArray(StringArray_t*);

    vector<string> data_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GESTRINGARRAY_H
