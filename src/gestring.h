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
class GAUSS_EXPORT GEString : public GESymbol
{
public:
    GEString();
    GEString(const string &);

    string getData() const;
    void setData(const string &);

    virtual int size() const;
    virtual void clear() { data_.clear(); } /**< Sets the string to empty. */
    virtual string toString() const;

private:
    GEString(String_t*);

    bool setString(String_t*);

    string data_;

    friend class GAUSS;
};

#endif // GESTRING_H
