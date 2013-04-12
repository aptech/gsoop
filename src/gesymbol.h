#ifndef GESYMBOL_H
#define GESYMBOL_H

#include <stdlib.h>
#include "gauss.h"
#include <string>
#include <iostream>
#include <vector>
using namespace std;

/**
  * Abstract parent class for all symbol types.
  */
class GESymbol
{
public:
    virtual int getRows();      /**< Return row count. */
    virtual int getCols();      /**< Return column count. */
    virtual bool isComplex();   /**< Return if data is complex. Applies to GEArray and GEMatrix only. */

    virtual int size();         /**< Return element count or in case of GEString, string length. */
    virtual void clear() {}     /**< Clear all corresponding symbol data. Does not clear from workspace. */

    virtual string toString() { return ""; } /**< Returns a string representation of this object. */

protected:
    GESymbol();
    virtual ~GESymbol();

    virtual void setRows(int);
    virtual void setCols(int);
    virtual void setComplex(bool);

    int rows_;
    int cols_;
    bool complex_;
};

#endif // GESYMBOL_H
