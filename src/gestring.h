#ifndef GESTRING_H
#define GESTRING_H

#include "gesymbol.h"
#include <stdio.h>
#include <string>
using namespace std;

/**
 * GAUSS String Symbol type. For all intents and purposes, this is a standard string
 * wrapper.
 *
 */
class GEString : public GESymbol
{
public:
    GEString();
    GEString(string);

    string getData();
    void setData(string);

    virtual int size();
    virtual void clear() { data_.clear(); } /**< Sets the string to empty. */
    virtual string toString();

private:
    GEString(String_t*);

    bool setString(String_t*);

    string data_;

    friend class GAUSS;
};

#endif // GESTRING_H
