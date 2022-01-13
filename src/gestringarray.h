#ifndef GESTRINGARRAY_H
#define GESTRINGARRAY_H

#include "gesymbol.h"
#include <stdio.h>
#include <vector>
#include <string>


/**
 * GAUSS String Array Symbol type. Represents a standard std::string array. Data is stored
 * internally as a std::vector.
 *
 */
class GAUSS_EXPORT GEStringArray : public GESymbol
{
public:
    GEStringArray();
    GEStringArray(VECTOR_DATA(std::string) data);
    GEStringArray(VECTOR_DATA(std::string) data, int rows, int cols);
    void setData(VECTOR_DATA(std::string) data, int, int);
    bool setElement(const std::string &value, int index);
    bool setElement(const std::string &value, int row, int col);

    std::string getElement(int index) const;
    std::string getElement(int row, int col) const;
    std::vector<std::string> getData() const;

    virtual std::string toString() const;
    virtual int size() const { return data_.size(); }
    virtual void clear() { data_.clear(); data_.resize(1); setRows(1); setCols(1); }

    StringArray_t* toInternal();

#ifdef SWIGPHP
    int position_;
#endif

private:
    GEStringArray(StringArray_t*);
    bool fromStringArray(StringArray_t*);

    std::vector<std::string> data_;

    friend class GAUSS;
    friend class GAUSSPrivate;
};

#endif // GESTRINGARRAY_H
